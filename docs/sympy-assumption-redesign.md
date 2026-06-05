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

| Field | Lives on | Purpose | Single source of truth |
|---|---|---|---|
| `m_assumption` (`numeric_assumption_manager`) | `expression` base | Numeric facts (positive, real, integer, ...) on Symbols. Renamed `leaf_facts_` conceptually; field name stays for ABI / minimal diff. | scalar Symbols, t2s Symbols (via forwarding) |
| `tensor_leaf_facts_` (renamed from `m_tensor_algebra_assumptions`) | `tensor_expression` | Tensor algebra facts (orthogonal, PD, PSD) on tensor Symbols. | tensor Symbols only |
| `m_tensor_space` (`tensor_space` variant) | `tensor_expression` | Perm/trace structural classification (Sym, Skew, Vol, Dev, Minor, ...). Single-valued by storage (`std::variant`). | Symbol assertion via `assume_symmetric` etc. OR derived-cache populate |
| `derived_cache_` | `tensor_expression` | Lazily-populated derived facts from the structural visitor. Mirrors `m_tensor_space` shape (single variant — no constant claims multiple variant alternatives at once, except `tensor_zero` which is special-cased). | The structural visitor |

The `inferred_` flag on `numeric_assumption_manager` (today an ad-hoc derived
marker) is subsumed by `derived_cache_` in step 3.

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
4. `is_symmetric` gains an explicit `MinorMajor` branch — `classify_space`
   maps MinorMajor to `Other`, but the rank-4 fully-symmetric case (which
   identity_tensor rank-4 produces) must answer true. Same cross-mechanism
   pattern as the existing PD check.
5. `tensor_to_scalar_zero` already pre-annotates in its constructor
   (existing code at #261). No change needed; documented as the model.
6. `levi_civita_tensor` left untouched. The current `tensor_space::perm`
   variant has no "totally antisymmetric" alternative; revisit if 1.1 needs it.

**Design rationale**: same pattern as `tensor_to_scalar_zero`/`tensor_to_scalar_one`
(#261). Eager constructor write to the existing storage avoids introducing a
`derived_cache_` field or a structural visitor until step 3 has compound
consumers that actually walk children. YAGNI applied — the visitor scaffolding
moves to step 3.

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

**Estimated test count delta**: +25 (1295 → 1320).

### Step 3 — Structural visitor + per-wrapper migration

**Note**: compound walks like `is_symmetric(I + I)` and `is_symmetric(α·I)`
ALREADY work today via the existing per-wrapper space-propagation in
n_ary_tree (binary add) and tensor_scalar_mul. Step 3 doesn't add this
behavior — it consolidates the scattered per-constructor writes into a
unified visitor. The migration is a refactor + memoization layer, not a
behavior change.

Today, several constructors and factories write into the assumption stores
directly. Step 3 introduces a structural visitor over tensor expressions and
moves each per-wrapper write into a corresponding visitor arm.

A `derived_cache_` field on `tensor_expression` (mirror shape of
`m_tensor_space`) memoizes the visitor's per-query computation. The bridge
in `is_*` helpers reads in order:

1. `tensor_zero` short-circuit (stays in the helper — the variant can't
   express Sym ∧ Skew simultaneously, so this can't migrate into the
   variant-based cache)
2. `m_tensor_algebra_assumptions` (user-asserted PD/PSD/orthogonal facts)
3. `m_tensor_space` (user-asserted structural classification OR closed-form
   constant pre-annotation from step 2)
4. `derived_cache_` (visitor-populated derived facts)

**Step 3 should be split into three sub-PRs** to keep review manageable:

- **3a**: `derived_cache_` field + structural visitor scaffolding +
  leaf-facts rename. No call-site migration; existing per-constructor
  writes stay. Visitor exists but is only invoked through new test
  consumers.
- **3b**: Per-wrapper migration. Move `tensor_functions.h:391,405`
  (trans orthogonal), `tensor_inv.h:79,83` (inv PD/PSD + Sym implication)
  into visitor arms.
- **3c**: Propagator rewrite. The scalar_assumption_propagator.cpp
  changes are non-trivial and deserve their own review focus.

**Migration sites** (verified by grep on `2026-06-05`):

| Site | Today writes | Becomes |
|---|---|---|
| `tensor_functions.h:391, 405` | trans() factory inserts `orthogonal{}` into result's `m_tensor_algebra_assumptions` | Visitor arm for `permute_indices_wrapper`: emit `orthogonal{}` into `derived_cache_` (or `tensor_leaf_facts_` if asserted) if child is orthogonal |
| `tensor_inv.h:79` | inv() ctor inserts `positive_definite{}` / `positive_semidefinite{}` | Visitor arm for `tensor_inv`: emit PD/PSD into `derived_cache_` if child has it |
| `tensor_inv.h:83` | inv() ctor calls `set_symmetric_unless_more_specific(this)` (writes `m_tensor_space`) | Visitor arm for `tensor_inv`: emit Sym into `derived_cache_` if child is PD/PSD |
| `tensor_to_scalar_zero.h:31` | t2s_zero ctor calls `a.set_inferred()` on all numeric assumptions | Deleted; t2s `is_*` helpers short-circuit on `is_same<tensor_to_scalar_zero>` (step 2 helper change) |
| `src/.../scalar_assumption_propagator.cpp:807` | propagator visitor calls `set_inferred()` after writing | Propagator writes into `derived_cache_` directly (or its scalar equivalent — see "scalar derived cache" below); `set_inferred()` retired once no writer remains |

**Scalar derived cache**: scalar `m_assumption` (`numeric_assumption_manager`)
becomes the leaf-facts store on scalar Symbols. We don't add a separate
`derived_cache_` for scalar; the propagator path writes directly into the
result holder's `m_assumption` (today's behavior) but loses the `inferred_`
flag. The flag was only consumed by t2s `det()` PD-propagation and one
introspection test — both can use a static `is_inferred_from(holder)` helper
that re-runs the inference, or we accept the loss of the flag entirely if no
caller depends on the distinction. **Decision deferred to step 3 implementation.**

**Behavior change in step 3**: queries on derived facts may differ in ordering
(cache vs. eager write) but final answers identical. Existing lock-ins from
α-2a/b/c/d still pass.

### Step 4 — Tighten `assume_*` to throw on non-Symbols

After step 3, all derived facts flow through the visitor / derived cache. At
this point we make `assume_*` strict:

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
- All `is_*` query helpers use the same read-order bridge
  (leaf-facts → asserted-space → derived-cache).
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

1. **Scalar derived cache** (step 3): keep `inferred_` flag, retire it, or
   replace with a `is_inferred_from(holder)` re-inference helper. Decided
   during step 3 implementation; default to retire unless a consumer surfaces.
2. **Levi-Civita classification**: today's `tensor_space` variant doesn't have
   a "totally antisymmetric" alternative. Step 2 arm is a no-op for
   `levi_civita_tensor`. If a 1.1 use case demands `is_totally_antisymmetric()`,
   add a new variant alternative and revisit.
3. **`identity_tensor` at non-rank-2/non-rank-4**: undefined for higher ranks
   in current code. Step 2 arm should gate on rank and leave higher-rank
   identity unclassified (no fact emission).

---

## Verified architectural claims (as of 2026-06-05)

- `is_symbol()` canonical override site: `symbol_base` at line 28. `tensor` /
  `scalar` inherit. Do not re-override in concrete leaves.
- `tensor_zero` never appears as a subterm — all 30+ `make_expression<tensor_zero>`
  sites are collapse-rule results, never wrapped as a child.
- `m_tensor_space::perm` is `std::variant<General, Symmetric, Skew, Young,
  Minor, Major, MinorMajor>` — structurally single-valued. Confirms
  `derived_cache_` can mirror this shape (no need for a multi-fact set).
- Five non-`assume_*` write sites in the codebase, listed in step 3 migration
  table. All five are addressed.

---

## Risks

1. **Visitor invocation cost**: each `is_*` query that hits the visitor
   populates `derived_cache_` lazily. First query is O(walk-cost), subsequent
   queries are O(1). For deep trees this could be noticeable. Mitigation:
   benchmark step 2 against step 1 on representative expressions; if
   regression > 5%, add eager-cache-populate at construction time for hot
   wrappers.
2. **Step 3 propagator migration touches `scalar_assumption_propagator.cpp`**,
   a non-trivial file. Plan was "2 sites", reality is 5 including the
   propagator. Mitigation: dedicate one PR to the propagator alone; don't
   bundle with the wrapper-ctor migrations.
3. **`inferred_` flag retirement (step 3)**: if any external consumer
   distinguishes asserted vs. inferred, removing the flag breaks them. The
   one known consumer (#259 det-PD propagation) doesn't actually read the
   flag — it just sets it as future-proofing. Audit before retiring.
4. **Backward compatibility**: `assume_*` helpers stay as wrappers in step 5,
   but they now THROW on non-Symbols (step 4). Existing user code calling
   `assume_positive_definite(A * B)` breaks at runtime. Mitigation: changelog
   entry, migration note in 1.0 release.

---

## Test surface summary

| Step | Tests added | Total |
|---|---|---|
| 0 | 0 (reverts) | 1293 |
| 1 | +2 (move-ctor lock-ins) | 1295 |
| 2 | +6 to +8 (closed-form short-circuits + visitor arms) | ~1302 |
| 3 | +5 to +10 (per-wrapper visitor-arm equivalence) | ~1312 |
| 4 | +10 to +15 (throwing-`assume_*` negative cases) | ~1325 |
| 5 | +5 to +8 (`assumption()` direct method) | ~1333 |
| 6 | +3 to +5 (cross-domain bridge consistency) | ~1338 |
| 7 | 0 (cleanup) | ~1338 |
