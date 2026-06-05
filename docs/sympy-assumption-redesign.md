# SymPy-style assumption-system redesign

**Status**: in progress on branch `sympy-assumption-redesign`.
**Decided model**: SymPy's. Only **Symbols** (named leaves) carry user-asserted
facts; **compounds** derive facts from structure + leaf facts. **Constants** get
facts from their type.
**Decided failure mode**: strict throw. Calling `assumption()` on a non-Symbol
throws `invalid_assumption_error`.

This doc tracks the multi-step migration. Each step is one PR. Behavior is
expected to be unchanged until step 3 onwards.

---

## Three (effective four) leaf categories

| Category | Examples | Fact source |
|---|---|---|
| Symbol (`is_symbol() == true`) | `tensor("A",3,2)`, `scalar("x")` | `leaf_facts_` — user-asserted via `assume_*` / `assumption()` |
| Closed-form constant | `tensor_zero`, `identity_tensor`, `tensor_to_scalar_zero`, scalar literals | Decided by the type (and rank/dim) |
| Parameterized constant | `tensor_projector(P_vol)`, `levi_civita_tensor(d)` | Decided by parameters |
| Compound | `tensor_add`, `tensor_inv`, `permute_indices_wrapper`, ... | Derived from children + structural rule (visitor) |

**Invariant relied upon throughout this plan**: `tensor_zero` (and
`tensor_to_scalar_zero`) **never appear as a subterm of a compound expression**.
Every operator and factory that would produce a zero subterm collapses to a
top-level `tensor_zero` instead. Verified by grep over `make_expression<tensor_zero>`
sites — every site is a collapse-rule result, never a subterm child.

Consequence: `tensor_zero` only needs to answer assumption queries when the
**user holds it directly** as a query target. It never appears in a visitor
walk. This lets us special-case zero with a one-line helper short-circuit
rather than baking it into the structural visitor (see step 2).

---

## Three storage layers (final shape)

After migration, every `expression` node carries:

| Field | Lives on | Purpose | Writers |
|---|---|---|---|
| `m_assumption` (`numeric_assumption_manager`) | `expression` base | Numeric facts (positive, real, integer, ...) on Symbols. Renamed `leaf_facts_` conceptually; field name stays for ABI / minimal diff. | scalar `assume_*`, t2s constants' ctors (via forwarding) |
| `tensor_leaf_facts_` (renamed from `m_tensor_algebra_assumptions`) | `tensor_expression` | Tensor algebra facts (orthogonal, PD, PSD) on tensor Symbols and propagated to compound wrappers. | `assume_*` helpers, compound-wrapper ctors via visitor (step 3b) |
| `m_tensor_space` (`tensor_space` variant) | `tensor_expression` | Perm/trace structural classification (Sym, Skew, Vol, Dev, Minor, MinorMajor, ...). Single-valued by storage (`std::variant`). | `assume_*` helpers, closed-form constant ctors (step 2), compound-wrapper ctors via visitor (step 3b) |

**No `derived_cache_` field**. The previous plan had one as a lazy-evaluation
layer; review showed it has no real consumer after step 2's eager
constructor pre-annotation. Dropped in favor of writing directly into the
existing storage.

`m_tensor_space` on closed-form constants is immutable post-construction:
`clear_space()` is virtualized in `tensor_expression` and overridden to
no-op in `identity_tensor` and `tensor_projector`. Same override pattern
applies to any future closed-form constant.

The `inferred_` flag on `numeric_assumption_manager` (today an ad-hoc derived
marker, used only by t2s `det()` PR #259 path) is retired in step 3c when
the propagator rewrite no longer needs it.

---

## Step-by-step plan

### Step 0 — Revert β-4 PR, keep base-ctor fix only ✅ done

Background: PR #272 ("β-4 substitution annotation propagation") had a base-ctor
fix (legitimate) and a `propagate_outer_annotations` helper (aliasing-mutation
bug, contradiction surface). Reverted everything except the base-ctor fix.

### Step 1 — Scaffolding (no behavior change) ✅ done

- `expression(expression const&)` / `expression(expression&&)` now propagate
  `m_assumption`. Without this every copy/move silently dropped asserted facts.
  Fix is `INVARIANT`-safe: hash is content-addressed and does NOT fold in
  assumptions, so copying the cached hash remains valid.
- `invalid_assumption_error` exception type in `core/cas_error.h`. Thrown by
  step 5's `assumption()` method on non-Symbols.
- `expression::is_symbol()` virtual, default `false`. Symbols override to
  return `true`. Canonical override site: **`symbol_base`** (`scalar` and
  `tensor` inherit from it). Do not re-override in the concrete leaf class.
- `tensor_to_scalar_scalar_wrapper` is a **transparent Symbol forwarder**: its
  `is_symbol()` returns its inner scalar's answer. Any future similar bridge
  wrapper follows the same pattern.
- Lock-in tests: `ScalarSymbolMovePreservesAssumptions`,
  `TensorSymbolMovePreservesAssumptions`. These caught a real `tensor` move-ctor
  bug discovered in the step-1 critical review (the by-name ctor route silently
  dropped `m_assumption`). Move ctor now chains
  `base(std::move(static_cast<base&&>(data)))` correctly.

Test count: 1293 → 1295.

### Step 2 — Closed-form constants answer `is_*` queries correctly

**Deliverables**:

1. `tensor_zero` short-circuit in `is_symmetric` / `is_skew`:
   `if (is_same<tensor_zero>(h)) return true;`. Two structural facts must
   coexist (Sym AND Skew), and the existing `m_tensor_space::perm`
   storage is a `std::variant` — single-valued. Pre-annotation can't
   express the simultaneity. The helper-level branch is forced by the
   storage shape, not by a Sym/Skew rivalry per se.
2. `identity_tensor` constructor pre-annotates `m_tensor_space`:
   - rank-2 → `{Symmetric, AnyTraceTag}`
   - rank-4 → `{MinorMajor, AnyTraceTag}`
   Each is a single-valued classification, so the variant works. Defaulted
   copy/move ctors preserve the annotation via `tensor_expression`'s 1-arg
   copy/move (the form that copies `m_tensor_space`).
3. `tensor_projector` single-sources its space from `m_tensor_space` (the
   old `space_` member is removed; its `space()` accessor derefs the
   optional and returns the value reference). This eliminates the
   drift-between-stores hazard surfaced in the step-2 review.
4. `is_symmetric` gains an explicit `MinorMajor` branch (gated on
   `AnyTraceTag` to reject the structurally invalid `{MinorMajor, VolumetricTag}`
   combination) — `classify_space` maps MinorMajor to `Other`, but the rank-4
   fully-symmetric case (which identity_tensor rank-4 produces) must answer
   true. Same cross-mechanism pattern as the existing PD check.
5. `clear_space()` virtualized in `tensor_expression`; overridden as no-op
   in `identity_tensor` and `tensor_projector`. Closed-form constants'
   structural classification is intrinsic to the type and cannot be
   removed via a base-class mutator. Without this, a caller invoking
   `clear_space()` through a `tensor_expression*` would leave projector's
   `m_tensor_space` empty and the next `space()` call would be release-mode
   UB (the projector's `space()` derefs the optional).
6. `tensor_to_scalar_zero` already pre-annotates in its constructor
   (existing code at #261). No change needed; documented as the model.
7. `levi_civita_tensor` left untouched. The current `tensor_space::perm`
   variant has no "totally antisymmetric" alternative; revisit if 1.1 needs it.

**Design rationale**: same pattern as `tensor_to_scalar_zero`/`tensor_to_scalar_one`
(#261). Eager constructor write to the existing storage avoids introducing a
`derived_cache_` field or a structural visitor. Compound walks already work
today via per-wrapper construction-time propagation; step 3 consolidates
that scattered logic into one visitor without adding a new storage layer.

**Test deliverables**:
- `is_symmetric(tensor_zero{3,2}) == true` + `is_skew(tensor_zero{3,2}) == true`
  + the simultaneity is_symmetric ∧ is_skew lock-in
- `is_symmetric(identity_tensor{3,2}) == true`
- `is_symmetric(identity_tensor{3,4}) == true` (rank-4 minor identity via
  the new MinorMajor branch in is_symmetric)
- `is_minor_major(identity_tensor{3,4}) == true`
- `is_volumetric(P_vol(3)) == true`, `is_deviatoric(P_dev(3)) == true`,
  `is_skew(P_skew(3)) == true`, `is_symmetric(P_sym(3)) == true`
- Negative matrix: `is_orthogonal(I)`, `is_positive_definite(I)`,
  `is_volumetric(I)`, etc. all false for each constant
- Move/copy preservation: identity_tensor through copy and move ctors keeps
  its m_tensor_space annotation (rank-2 Sym AND rank-4 MinorMajor branches)
- Hash invariance: identity_tensor with and without the annotation hash equal
  (m_tensor_space is not part of the content-addressed hash)
- Contradictory assertion: `assume_skew(P_vol(3))` overwrites Vol with Skew
- Compound regression: `is_symmetric(I + I)`, `is_symmetric(α·I)`,
  `is_symmetric(trans(I))` all true via the existing per-wrapper
  space-propagation

**Behavior change**: `is_symmetric(I)` and `is_volumetric(P_vol)` used to
return false (despite being mathematically true); they now return true. No
downstream simplifier currently dispatches on these queries for `I` or
`P_*`, so this is purely an answer-correctness improvement.

**Estimated test count delta**: +34 (1295 → 1329).

### Step 3 — Consolidate per-wrapper propagation into a visitor

**Decided architecture (post-review)**: NO `derived_cache_` field, NO new
storage layer. Step 3 is pure refactoring — move existing per-constructor
propagation logic into a structural visitor that writes into the SAME
storage already used today (`m_tensor_space`, `m_tensor_algebra_assumptions`).

The previous plan had `derived_cache_` as a memoization layer for visitor
output. After step 2 eagerly pre-annotates closed-form / parameterized
constants in their constructors AND every compound wrapper already
propagates space at construction, there is no query path that needs lazy
visitor evaluation. Adding the cache solved a problem step 2 obviates.
YAGNI: dropped.

**Compound walks already work today.** `is_symmetric(I + I)`,
`is_symmetric(α·I)`, `is_symmetric(trans(I))` all return true via existing
per-wrapper space-propagation in n_ary_tree (binary add), tensor_scalar_mul,
tensor_negative, etc. Step 3 doesn't add new behavior — it centralizes the
propagation logic into one auditable place.

#### Read order (actual, per-helper)

The original plan implied a uniform bridge across all `is_*` helpers. In
reality each helper consults a different subset of storage; this is by
design (e.g. PD only implies symmetry, not skew). Documented as-is:

| Helper | Reads (in order) |
|---|---|
| `is_symmetric` | tensor_zero short-circuit → `m_tensor_algebra_assumptions` (PD/PSD) → `m_tensor_space` (Sym/Vol/Dev OR MinorMajor+AnyTraceTag) |
| `is_skew` | tensor_zero short-circuit → `m_tensor_space` (Skew) |
| `is_volumetric` / `is_deviatoric` / `is_minor` / `is_major` / `is_minor_major` | `m_tensor_space` only |
| `is_orthogonal` / `is_positive_definite` / `is_positive_semidefinite` | `m_tensor_algebra_assumptions` only |

The `tensor_zero` short-circuit stays in `is_symmetric` and `is_skew`
because the `tensor_space::perm` variant is single-valued and cannot
express Sym ∧ Skew simultaneously.

#### Compound-propagation sites today (the step-3 migration scope)

Audit on 2026-06-05 — **16 compound-propagation sites** exist (not 5 as
earlier drafts claimed):

| Site count | Files |
|---|---|
| 4 | `operators/tensor/tensor_add.h` (n_ary join) |
| 3 | `wrappers/tensor_inv.h` (PD/PSD propagation + Sym implication) |
| 2 | `tensor_operators.h` (binary add `trans(A) + (-A) → Skew`) |
| 2 | `wrappers/tensor_pow.h` (pow preserves child space, pow(A,2k)→Sym) |
| 2 | `tensor_functions.h:391,405` (trans propagates orthogonal) |
| 2 | `operators/scalar/tensor_scalar_mul.h` (scalar·tensor preserves space) |
| 1 | `tensor_negative.h` (negation preserves space) |

Plus, separately, the t2s `set_inferred()` pattern in:
- `tensor_to_scalar_zero.h:31` (closed-form constant — subsumable into the
  short-circuit pattern from step 2)
- `src/.../scalar_assumption_propagator.cpp:807` (the scalar propagator)

#### Sub-PRs

- **3a**: Structural visitor scaffolding + leaf-facts rename
  (`m_tensor_algebra_assumptions` → `tensor_leaf_facts_` alias accessor).
  No migration yet; visitor exists with no consumers.
- **3b**: Migrate the 16 compound-propagation sites into visitor arms.
  Writes still target `m_tensor_space` / `m_tensor_algebra_assumptions`;
  the visitor IS the construction-time propagator, called from each
  wrapper's constructor. This eliminates the scattered set_space calls
  while preserving behavior.
- **3c**: Rewrite `scalar_assumption_propagator.cpp` to use the same
  visitor pattern as the tensor side. Non-trivial; deserves its own PR.

(Note: closed-form constant clear_space() is already a no-op per
step-2 review fix; the override pattern extends naturally to any future
closed-form constant.)

**Migration sites** (verified by grep on `2026-06-05`):

Each existing constructor-level propagation moves into a visitor arm that
writes into the SAME storage (`m_tensor_space` or `m_tensor_algebra_assumptions`).
No new storage layer. The 16 sites are listed in the audit above; the
representative cases:

| Site | Today writes | Becomes |
|---|---|---|
| `tensor_functions.h:391, 405` | trans() factory inserts `orthogonal{}` into result | Visitor arm for `permute_indices_wrapper` writes the same insert |
| `tensor_inv.h:27,29,42` | inv() ctor propagates child space + PD/PSD + Sym implication | Visitor arm for `tensor_inv` writes the same |
| `tensor_add.h:18,23,44,48` | n_ary add joins child spaces | Visitor arm for `tensor_add` writes the same |
| `tensor_scalar_mul.h:21,28` | scalar·tensor preserves rhs space | Visitor arm for `tensor_scalar_mul` writes the same |
| `tensor_to_scalar_zero.h:31` | t2s_zero ctor calls `set_inferred()` | Deleted; t2s `is_*` helpers short-circuit on `is_same<tensor_to_scalar_zero>` (step 2 pattern) |
| `src/.../scalar_assumption_propagator.cpp:807` | propagator visitor calls `set_inferred()` after writing | Propagator writes into `m_assumption` directly; `set_inferred()` retired once no writer remains |

**Inferred flag retirement**: the `inferred_` flag was only consumed by t2s
`det()` PD-propagation (#259) and one introspection test. Both can use a
static `is_inferred_from(holder)` helper that re-runs inference, or we
accept the loss of the flag entirely. Decision deferred to step 3c.

**Behavior change in step 3**: none. This is pure refactoring. All
α-2a/b/c/d lock-ins continue to pass; the same writes happen at the same
construction points, just through a centralized dispatcher.

### Step 4 — Tighten `assume_*` to throw on non-Symbols

After step 3, all derived facts flow through the visitor. At this point we
make `assume_*` strict:

- `assume_positive_definite(holder)` — guards `holder.get().is_symbol() == true`,
  else throws `invalid_assumption_error`.
- Same for `assume_orthogonal`, `assume_symmetric`, `assume_skew`,
  `assume_volumetric`, `assume_deviatoric`, and their scalar siblings.
- The `tensor_to_scalar_scalar_wrapper` forwarder makes `assume_positive(wrapped_x)`
  work transparently (`is_symbol()` forwards through).
- Tests: lock-in that `assume_positive_definite(A+B)` throws when A,B are
  Symbols; succeeds when called on a tensor Symbol directly.

**Test count delta**: +10 to +15 (negative cases for each `assume_*` helper).

### Step 5 — Add `assumption()` method on `expression_holder`

The SymPy-style entry point. Replaces the assorted `assume_*` helpers with one
consistent API:

```cpp
holder<tensor_expression> A{"A", 3, 2};
A.assumption(symmetric{}, positive_definite{});  // multi-fact assertion
```

- Symbol → writes to `tensor_leaf_facts_` (or `m_tensor_space` for structural
  facts like Sym/Skew/Vol/Dev).
- Non-Symbol → throws `invalid_assumption_error`.
- `assume_*` helpers stay as thin wrappers around `assumption()` for backwards
  compatibility within numsim-cas; do not advertise them in 1.0 docs.

### Step 6 — Cross-domain consistency sweep

- All `assume_*` helpers across scalar / tensor / t2s use the same
  is-Symbol-or-throw guard.
- All `is_*` query helpers follow the per-helper read order documented in
  step 3 above (no uniform bridge — each helper's read order is intentionally
  scoped to which facts can imply its predicate).
- All closed-form constants (`tensor_zero`, `tensor_to_scalar_zero`,
  `identity_tensor`, scalar literals) answer queries via either the structural
  visitor arm OR a helper short-circuit, consistently per category.

### Step 7 — Cleanup and 1.0 lockdown

- Delete dead code: `set_inferred()` if no remaining caller, deprecated
  accessor `tensor_algebra_assumptions()` (rename complete).
- Doc updates: `docs/tensor.md`, `docs/scalar.md`, `docs/tensor-to-scalar.md`
  get a "Assumptions" section pointing at the SymPy model.
- Mark `assumption()` as part of the stable public API.

---

## Open decisions still to make

1. **Inferred flag retirement** (step 3c): keep `inferred_` flag, retire
   it, or replace with a `is_inferred_from(holder)` re-inference helper.
   Default: retire unless a consumer surfaces during step 3c implementation.
2. **Levi-Civita classification**: today's `tensor_space` variant doesn't have
   a "totally antisymmetric" alternative. Step 2 left `levi_civita_tensor`
   untouched. If a 1.1 use case demands `is_totally_antisymmetric()`, add
   a new variant alternative and revisit.
3. **`identity_tensor` at non-rank-2/non-rank-4**: undefined for higher
   ranks in current code. The rank-6 identity is mathematically
   `δ_il·δ_jm·δ_kn` which has full symmetry, but the variant has no
   matching alternative. Tracked as the rank-6 sentinel test.

---

## Verified architectural claims (as of 2026-06-05)

- `is_symbol()` canonical override site: `symbol_base` at line 28. `tensor` /
  `scalar` inherit. Do not re-override in concrete leaves.
- `tensor_zero` never appears as a subterm — all 30+ `make_expression<tensor_zero>`
  sites are collapse-rule results, never wrapped as a child.
- `m_tensor_space::perm` is `std::variant<General, Symmetric, Skew, Young,
  Minor, Major, MinorMajor>` — structurally single-valued. Variant cannot
  express Sym ∧ Skew simultaneously, hence the `tensor_zero` helper
  short-circuit.
- 16 compound-propagation sites in the codebase (audited above) — all are
  step-3 migration targets. The earlier doc claim of "5 sites" was
  undercounted.
- Closed-form constants (`identity_tensor`, `tensor_projector`) override
  `clear_space()` to no-op; their `m_tensor_space` is immutable
  post-construction (assertion-overwrite via `set_space()` still permitted).

---

## Risks

1. **Step 3 site count larger than initially scoped**: 16 compound
   propagation sites, not 5. Mitigation: 3b can split further if review
   load is too high (e.g. 3b-i for n_ary nodes, 3b-ii for unary wrappers).
2. **`inferred_` flag retirement (step 3c)**: if any external consumer
   distinguishes asserted vs. inferred, removing the flag breaks them. The
   one known consumer (#259 det-PD propagation) doesn't actually read the
   flag — it just sets it as future-proofing. Audit before retiring.
3. **Backward compatibility**: `assume_*` helpers stay as wrappers in
   step 5, but they now THROW on non-Symbols (step 4). Existing user code
   calling `assume_positive_definite(A * B)` breaks at runtime. Mitigation:
   changelog entry, migration note in 1.0 release.

---

## Test surface summary

| Step | Tests added | Total |
|---|---|---|
| 0 | 0 (reverts) | 1293 |
| 1 | +2 (move-ctor lock-ins) | 1295 |
| 2 | +34 (closed-form short-circuits, pre-annotations, negative matrices, hash, clear_space no-op, contradictory assume, compound regression, round-trip) | 1329 |
| 3 | +5 to +10 (visitor-arm-equivalence lock-ins after migration) | ~1339 |
| 4 | +10 to +15 (throwing-`assume_*` negative cases) | ~1352 |
| 5 | +5 to +8 (`assumption()` direct method) | ~1360 |
| 6 | +3 to +5 (cross-domain bridge consistency) | ~1365 |
| 7 | 0 (cleanup) | ~1365 |
