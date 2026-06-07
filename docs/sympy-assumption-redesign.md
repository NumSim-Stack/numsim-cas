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
| `tensor_leaf_facts_` (renamed from `m_tensor_algebra_assumptions`) | `tensor_expression` | Tensor algebra facts (orthogonal, PD, PSD) on tensor Symbols and propagated to compound wrappers. | `assume_*` helpers, compound-wrapper ctors |
| `m_tensor_space` (`tensor_space` variant) | `tensor_expression` | Perm/trace structural classification (Sym, Skew, Vol, Dev, Minor, MinorMajor, ...). Single-valued by storage (`std::variant`). | `assume_*` helpers, closed-form constant ctors (step 2), compound-wrapper ctors |

**No `derived_cache_` field**. The previous plan had one as a lazy-evaluation
layer; review showed it has no real consumer after step 2's eager
constructor pre-annotation. Dropped in favor of writing directly into the
existing storage.

`m_tensor_space` on closed-form constants is immutable post-construction:
`clear_space()` is virtualized in `tensor_expression` and overridden to
no-op in `identity_tensor` and `tensor_projector`. Same override pattern
applies to any future closed-form constant.

The `inferred_` flag on `numeric_assumption_manager` (today an ad-hoc derived
marker, used only by t2s `det()` PR #259 path) is retired as part of a
later cleanup step (no urgency; harmless today).

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

### Step 3 — Extract shared propagation helpers (focused)

**Scope (post-implementation audit)**: 16 sites counted by raw grep but
only ONE pattern is genuinely duplicated across wrappers: the
**pass-through unary preserve** used identically by `tensor_negative` and
`tensor_scalar_mul`. Every other "site" is either:

- Single-caller logic with no duplication (`tensor_pow` closure rules,
  `tensor_inv` PD/PSD inheritance — each lives in exactly one wrapper)
- Already centralized within its own header (`tensor_add` n-ary join — 4
  sites in `join_child_space`, one logical operation)
- 2-line inline code clearly tied to its surrounding fold (binary add/sub
  `trans(A) + (-A) → Skew`, `trans()` orthogonal propagation)

Extracting these into a visitor or centralized helper produces churn
without consolidation benefit. The doc's earlier "16-site migration"
framing was technically accurate but pragmatically misleading — only one
extraction is a genuine win.

**Decided architecture**: NO `derived_cache_` field, NO visitor class,
NO new storage layer. Step 3 is one focused refactor.

**Compound walks already work today.** `is_symmetric(I + I)`,
`is_symmetric(α·I)`, `is_symmetric(trans(I))` all return true via existing
per-wrapper space-propagation in n_ary_tree (binary add), tensor_scalar_mul,
tensor_negative, etc. Step 3 doesn't add new behavior.

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

#### What step 3 actually delivers

**Single extraction**: a `structural_propagation` namespace in
`include/numsim_cas/tensor/structural_propagation.h` with
`preserve_unary(out, child)` — the pass-through pattern. Two wrappers
migrate to use it:

- `tensor_negative` — `−A` inherits A's classification
- `tensor_scalar_mul` — `α·A` inherits A's classification

Three isolated unit tests verify the helper works through each caller:
`PreserveUnaryCopiesChildSpace`, `PreserveUnaryNoopWhenChildHasNoSpace`,
`ScalarMulPreservesRhsSpace`.

**Not migrated and why**:

- `tensor_pow` — closure rules (Sym^n stays Sym, Vol stays Vol, Dev
  downgrades to Sym, Skew alternates so dropped). Single-caller unique
  logic; extraction adds indirection without consolidation.
- `tensor_inv` — PD/PSD inheritance + Sym implication. Single-caller;
  comments document the rationale at the call site.
- `tensor_add` (n-ary) — 4 sites in `join_child_space` are paths through
  ONE logical join operation, already centralized.
- `tensor_operators.h` binary add/sub — `trans(A) + (-A) → Skew` and
  symmetric sub case. Two sites, 1 line each, tightly tied to their
  surrounding fold; extraction would split tightly coupled code.
- `tensor_functions.h:391, 405` — trans propagates orthogonal. Two
  1-line writes, one conditional on a fold path.

#### Separately: t2s `set_inferred()` retirement (deferred)

The t2s constants pattern at `tensor_to_scalar_zero.h:31` and the
propagator at `src/.../scalar_assumption_propagator.cpp:807` use a
`set_inferred()` flag that was originally intended to compose with the
dropped `derived_cache_`. Retire as part of step 5 or 7 (no urgency;
the flag is harmless today).

**Behavior change in step 3**: none. The two migrated wrappers continue
to produce identical output. All α-2a/b/c/d lock-ins continue to pass.

### Step 4 — Tighten `assume_*` to throw on non-Symbols ✅ done

Strict SymPy-style: only Symbols accept user-asserted facts. Compounds,
constants, and wrappers throw `invalid_assumption_error`.

**Delivered**:

1. New shared guard `include/numsim_cas/core/require_symbol.h` —
   `detail::require_symbol(expr, fn_name)` throws if `expr.is_symbol() == false`.
2. Tensor `assume_*` helpers (10 total: symmetric, skew, volumetric,
   deviatoric, minor, major, minor_major, orthogonal, positive_definite,
   positive_semidefinite) call `require_symbol` first.
3. Scalar `assume(...)` overloads (11 total: positive, negative,
   nonnegative, nonpositive, nonzero, integer, even, odd, prime, rational,
   real_tag) call `require_symbol` first.
4. `scalar_constant` constructor now self-annotates numeric assumptions
   from its value (positive/negative/nonzero/integer/rational/real_tag).
   Previously test code did `assume(c5, positive{})` on a literal; that
   call now throws (literal is not a Symbol) AND is no longer needed —
   the value self-classifies. Same pattern as `tensor_to_scalar_zero/one`
   pre-annotation. Skips sign predicates for `std::complex` values
   (not orderable).
5. Step-2 overwrite tests (`AssumeSkewOverwritesSymmetricTag` on
   identity, `AssumeSkew/SymmetricOnP*` on projectors) updated: they
   used to lock in "user assertion overwrites pre-annotation" — under
   SymPy that contract is a category error. Now lock in throwing
   behavior + post-throw state preservation.
6. One existing test (`ScalarFixture.AbsSimplification`) updated to
   remove the now-redundant `assume(c5, positive{})` call.

`tensor_to_scalar_scalar_wrapper`'s `is_symbol()` forwarder (added in
step 1) ensures wrapped scalar Symbols remain assumable transparently.

**Net test surface change**: 11 added + 3 step-2 tests semantically
rewritten (overwrite-contract → throw-contract) + 1 existing test had
one line removed. Cumulative count 1333 → 1344, but the "11" headline
understates the rewritten-test impact — the original overwrite contract
is no longer documented anywhere in the test suite (intentionally; it
was a category error).

**Follow-up review fixes** (commit after a072dac):

- `annotate_from_value()` `noexcept` removed — `std::set::insert` can
  throw `std::bad_alloc`. Specifier was technically incorrect.
- Zero-spelling consistency: `scalar_constant(0)`, `scalar_constant(0.0)`,
  `scalar_constant(rational_t{0,1})` now all carry the same fact set
  including `integer` and `rational`. Pre-fix, the double-zero branch
  diverged from the int branch. SymPy convention: zero is integer
  regardless of spelling; 0.0 is exact in IEEE 754, so safe to claim
  integer. Non-zero doubles still do NOT claim integer.
- Coverage adds: 8 new tests for scalar_constant value-derived
  assumptions (positive int, negative int, zero spelling parity,
  non-zero double NOT integer, rational with non-trivial denominator,
  complex no-real/no-sign) + `assume_minor_major` on compound throws +
  `assume(even{})` on compound throws.

**Public mutators bypass the guard** (deferred to step 7 cleanup):

`tensor_expression::set_space()`, `tensor_algebra_assumptions().insert(...)`,
and `numeric_assumption_manager::insert(...)` are public mutators. They
have ~28 legitimate internal call sites (tensor_add, tensor_inv,
tensor_pow, projection_tensor and identity_tensor ctors, projector
algebra simplifier, structural_propagation helpers, the `assume_*`
helpers themselves, and the dev/sym/vol/skew factories). These are all
correct ctor-time propagation — they are the mechanism the design uses
to carry space info through compound expressions.

The step-7 concern isn't that internal writers exist (they're necessary)
but that the visibility is public, so user code CAN bypass the
`require_symbol` guard. Step 7 should restrict visibility — either
protected + friend grants for the legitimate internal writers, or
rename + audit (`set_space_internal` with documented contract) —
without breaking the propagation pathway.

### Step 5 — Add `assumption()` method on `expression_holder` ✅ done

The SymPy-style entry point. Replaces the assorted `assume_*` helpers with one
consistent API:

```cpp
holder<tensor_expression> A{"A", 3, 2};
A.assumption(symmetric{}, positive_definite{});  // multi-fact assertion
```

- Symbol → writes to `tensor_leaf_facts_` (or `m_tensor_space` for structural
  facts like Sym/Skew/Vol/Dev).
- Non-Symbol → throws `invalid_assumption_error`.
- `assume_*` helpers remain available as the legacy entry point and
  ARE documented in the per-domain docs alongside `assumption()` — real
  user code will encounter them, and pretending they don't exist would
  hurt discoverability. (Earlier draft said "don't advertise"; that
  decision was reversed in step 7's doc audit.)

**Variadic-call API contract** (architect step-5 prep):

- **Return type**: `expression_holder<E>&` (returns `*this`). Enables
  SymPy-style fluent chaining: `A.assumption(symmetric{}).assumption(positive_definite{})`.
  The 0-fact case also returns `*this` for consistency — same return type
  regardless of arity.
- **0 facts** (`A.assumption()`) — no-op, returns `*this`. Defensible for
  forwarding from generic code that may pass an empty parameter pack.
- **N facts** — no upper bound. C++ template parameter packs handle
  arbitrary arity; the runtime cost is linear in pack size.
- **Type-level filter** — accepted via a concept `assumption_fact<T>`
  that constrains the variadic parameter pack: `requires (assumption_fact<Args> && ...)`.
  The concept is satisfied by any type derivable from the
  `numeric_assumption`/`tensor_algebra_assumption` taxonomies — i.e.
  `positive{}`, `symmetric{}`, etc. Mistyped arguments
  (`A.assumption(42, "foo")`) become a clear "concept not satisfied"
  diagnostic instead of a deep template-error spew.
- **Domain dispatch**: single template on `expression_holder<E>`;
  per-fact-type dispatch (via tag matching on the fact's type) routes
  structural facts (Sym/Skew/Vol/Dev for tensor) to `m_tensor_space`
  and numeric/algebraic facts to the appropriate manager
  (`m_assumption` for scalar/t2s, `tensor_algebra_assumptions` for
  tensor). Per-domain free-function tag invoke handles the dispatch
  so the holder method body stays simple.

**Variadic implication-chain ordering** (architect step-5 prep):

Variadic assertion like `A.assumption(positive_definite{}, symmetric{})`
needs a well-defined order for cross-mechanism implications. PD implies
symmetric for real matrices — so the assertion `A.assumption(positive_definite{})`
ALONE should produce a symmetric A. But what about combinations?

Decided contract: **left-to-right, with each fact's full implication chain
run before the next fact is processed**. This matches today's per-helper
behavior (`assume_positive_definite` inserts PD, then PSD, then runs
`set_symmetric_unless_more_specific`, all before returning).

Trade-offs:
- Left-to-right is predictable and matches user-write order.
- Implication chains are idempotent (re-running has no effect), so the
  order rarely matters in practice — most combinations converge to the
  same final fact set regardless.
- Contradiction reporting: if `A.assumption(skew{}, positive_definite{})`
  is asserted, the user-write order determines the final space tag
  (Skew first, then PD's set_symmetric_unless_more_specific overwrites
  to Sym). Document this; throw on contradiction is OUT of scope for
  step 5 (would need a contradiction-detection pass).

Step 5 ALSO closes one architectural loose-end: today's `assume_*`
helpers each do their own implication chain; `assumption()` is the
opportunity to put the chain logic in one place. The per-helper
implementations become `assumption(fact)` calls.

### Step 6 — Cross-domain consistency sweep ✅ done

Verification (not new feature). The three invariants — uniform
`require_symbol` guard, per-helper documented read order, closed-form
constant query consistency — were all delivered piecewise in steps
2/3/4/5. Step 6 audits + pins them:

- ✅ Uniform `is-Symbol-or-throw` guard: 10/10 tensor `assume_*` helpers
  and 11/11 scalar `assume(...)` overloads call `detail::require_symbol`.
  T2s holders forward through the scalar dispatch via the wrapper
  overload (step 5). Verified by `Step6_ScalarAssumeUniformGuardSampling`
  and `TensorAlgebraStep6.TensorAssumeUniformGuardSampling`.
- ✅ Per-helper read order: documented in the step-3 doc table.
  Compile-time enforced by the per-helper code paths; no uniform-bridge
  refactor needed.
- ✅ Tensor closed-form constant query consistency:
  `Step6.ClosedFormConstantQueryConsistency` pins that `tensor_zero`
  (helper short-circuit), `identity_tensor` (ctor pre-annotation),
  and `tensor_projector` (ctor pre-annotation) all answer
  `is_symmetric` the same way. (Scalar `scalar_constant` and t2s
  `tensor_to_scalar_zero/one` consistency is covered by their
  domain-specific test batteries — they don't need a tensor-side sweep.)
- ✅ Cross-domain concept dispatch parity:
  `Step6_ConceptDispatchT2sParityWithScalar` pins that the
  `assumption_fact_for` concept produces the same answer through both
  scalar and t2s routes for supported, unsupported (`irrational`,
  `complex_tag`), and tensor-only (`Symmetric`) tags.
- ✅ T2s integration: `Step6_AssumeOnT2sWrapperRoutesToInnerScalar`
  verifies the variadic API on a t2s holder over a wrapped scalar Symbol
  routes through the wrapper's `apply_assumption` overload to the inner
  scalar's assumption set, observable via the original scalar holder.

### Step 7 — Cleanup and 1.0 lockdown ✅ done

Audit results:

- **`set_inferred()` is NOT dead code.** The scalar propagator at
  `src/numsim_cas/scalar/visitors/scalar_assumption_propagator.cpp:837`
  reads `.inferred()` as a cache flag — if set, the propagator
  short-circuits. Callers are the 11 scalar `assume()` overloads,
  `scalar_constant`'s ctor, `tensor_to_scalar_zero/one` ctors, and the
  t2s `tensor_to_scalar_functions.cpp` det() PD-propagation path
  (#259). Earlier rounds incorrectly assumed this was dead; the audit
  reversed that. The flag stays. Open decision #1 (retirement) is
  closed as "keep" — it's the propagator's memoization mechanism.
- **`tensor_algebra_assumptions()` rename never happened.** The earlier
  plan to rename to `tensor_leaf_facts_` was tied to the dropped
  derived_cache_ design (step 2 review). The current name is the
  long-term name. No deprecated accessor to delete.
- **Doc updates landed**: `docs/scalar.md`, `docs/tensor.md`, and
  `docs/tensor-to-scalar.md` all gained an "Assumptions" section
  documenting the variadic API, the legacy helpers, the supported tags
  + implication chains, closed-form constant pre-annotations, and the
  known query-limitation for t2s holders.
- **`assumption()` documented as the intended-stable public API**.
  No mechanical enforcement (no `[[stable]]` markers, no API-freeze
  CI gate) — the stability is a documentation contract, not a code
  contract. A future contributor can still change the signature; the
  domain doc + this scoping doc are the only "lockdown" surface.
  Legacy helpers (`assume(holder, tag)`, `assume_*(holder)`) remain
  documented and supported.

**Honest caveat on the "Cleanup and 1.0 lockdown" framing**: the audit
revealed no dead code to clean (`set_inferred()` is live, the
`tensor_algebra_assumptions()` rename never happened). What shipped is
domain-doc updates. No version bump, no `[[deprecated]]` markers, no
CHANGELOG entry. The scoping doc has been updated to reflect what was
actually delivered.

---

## Open decisions still to make

1. **Inferred flag retirement** ✅ resolved: **keep**. Step-7 audit
   surfaced the real reader in `scalar_assumption_propagator.cpp:837`
   — `inferred_` is the propagator's cache hit/miss flag, not dead
   code as earlier rounds assumed. Retirement is OFF the table.
2. **Levi-Civita classification**: today's `tensor_space` variant doesn't have
   a "totally antisymmetric" alternative. Step 2 left `levi_civita_tensor`
   untouched. If a 1.1 use case demands `is_totally_antisymmetric()`, add
   a new variant alternative and revisit.
3. **`identity_tensor` at non-rank-2/non-rank-4**: undefined for higher
   ranks in current code. The rank-6 identity is mathematically
   `δ_il·δ_jm·δ_kn` which has full symmetry, but the variant has no
   matching alternative. Tracked as the rank-6 sentinel test.
4. **T2s mixed-domain query inference**: `is_*` query helpers exist
   only for scalar holders. A `tensor_to_scalar_scalar_wrapper` around
   a scalar Symbol has its own (independent) `m_assumption` set. The
   wrapper does NOT forward queries to the inner scalar; users must
   unwrap via `wrapper.expr()` and query the inner scalar holder.
   The variadic `assumption()` WRITE path already routes through the
   wrapper (step 5); a future addition could add `is_*` overloads for
   t2s holders that forward through similarly. Tracked as the
   `Step6_T2sWrapperQueryRequiresInnerUnwrap` sentinel.

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
- 16 compound-propagation sites in the codebase (audited 2026-06-05). Of
  those, only ONE pattern has real duplication worth extracting (the
  pass-through unary preserve, used by tensor_negative and
  tensor_scalar_mul — migrated in step 3). The remaining 14 are
  single-caller unique logic, already centralized within their own
  headers, or 2-line inline code tightly coupled to surrounding folds.
  The earlier draft's framing of "16 migration targets" was technically
  accurate as a count but pragmatically misleading.
- Closed-form constants (`identity_tensor`, `tensor_projector`) override
  `clear_space()` to no-op; their `m_tensor_space` is immutable
  post-construction (assertion-overwrite via `set_space()` still permitted).

---

## Risks

1. **Step 3 over-scoped in earlier drafts**: the initial framing of "16
   propagation sites needing migration" turned out to overstate the
   refactoring opportunity — only the pass-through pattern (used by 2
   wrappers) had duplication worth extracting. Site count in itself is a
   misleading metric; the question is "how many sites share a pattern".
2. **`inferred_` flag retirement**: if any external consumer
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
| 3 | +3 (preserve_unary unit lock-ins) | 1332 |
| 4 | +10 to +15 (throwing-`assume_*` negative cases) | ~1352 |
| 5 | +5 to +8 (`assumption()` direct method) | ~1360 |
| 6 | +3 to +5 (cross-domain bridge consistency) | ~1365 |
| 7 | 0 (cleanup) | ~1365 |
