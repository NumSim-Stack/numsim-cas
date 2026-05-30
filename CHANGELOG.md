# Changelog

All notable changes to NumSim-CAS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and from v0.1.0 onward the project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Optional `NumSim_CAS::Parser` library (CMake option `NUMSIM_CAS_BUILD_PARSER=ON`, default OFF). Phase 1 of issue #214: error-class hierarchy (`parse_error` + 8 specialised subclasses) with snippet-with-caret rendering in `what()`; on-the-fly typed `symbol_table` with implicit scalar declarations, explicit `Name{rank,dim}` tensor declarations, redeclaration / cross-type collision detection. 18 lock-in tests gated on `NUMSIM_CAS_PARSER_ENABLED`. Phase 2 will add the PEGTL grammar and the parse entry points.
- DCO (Developer Certificate of Origin) enforcement via `.github/workflows/dco-check.yml`. Non-merge PR commits must carry a `Signed-off-by:` trailer. Closes part of #171.
- `NOTICE` file at the repo root asserting petlenz's copyright and the GPL-3.0 OR commercial dual-license model, with SPDX identifiers. Closes part of #171.
- `LICENSES/LicenseRef-Commercial.txt` so REUSE-compliant tooling can resolve the SPDX `LicenseRef-Commercial` identifier; points at COMMERCIAL.md for the actual acquisition path.
- Linear elasticity example (`examples/linear_elasticity.cpp`) demonstrating symbolic strain-energy → automatic differentiation → numerical evaluation. Includes programmatic assertions and a CTest entry so CI catches bit-rot. Closes #160, #172.
- Scalar overloads `log10`, `sinh`, `cosh`, `tanh`, `asinh`, `acosh`, `atanh` composed from existing primitives — no new AST nodes; differentiation derives automatically through the composition. Includes construction-time short-circuits (`sinh(0) → 0`, `cosh(-x) → cosh(x)` (even), `tanh(-x) → -tanh(x)` (odd), `acosh(1) → 0`, etc.) and a structural fix for atanh's previously-platform-dependent evaluation order. Closes #33.
- Lock-in test for `a * (1/b) == a/b` canonicalisation (already covered by existing infrastructure; pinned to prevent regression). Closes #49.
- `tag_invoke(mul_fn, …)` for `tensor × tensor_to_scalar` (and the symmetric pair) with full visitor coverage and a dedicated simplifier handling nested-mul collapse and scalar-coefficient bubbling. Closes #145.
- `tag_invoke(div_fn, tensor, t2s)` routing through `lhs × pow(rhs, -1)` for `tensor ÷ tensor_to_scalar`. Closes #147.
- Scalar comparison nodes (`lt`, `gt`, `le`, `ge`, `eq`, `ne`) producing Real-valued indicators (1.0 / 0.0) for the damage-activation idiom and the upcoming `if_then_else`. Closes #136.
- Scalar `max(a, b)` / `min(a, b)` AST nodes with `max()` / `min()` free functions in `scalar_std.h`. Construction-time folds: `max(x, x) → x`, numeric folding (`max(3, 5) → 5`), and commutative canonical form (operand sort by hash) so `max(x, y)` and `max(y, x)` compare equal. Visitor coverage across printer/latex_printer/evaluator/rebuild_visitor/contains_expression/assumption_propagator/limit_visitor. Differentiation throws `not_implemented_error` until `if_then_else` (#135) lands — the piecewise rule `d/dx max(a, b) = if_then_else(a > b, da/dx, db/dx)` requires that node. Closes #137.
- Constitutive-modelling primitives composed from existing nodes (no new AST): `macauley_plus(e) = max(e, 0)` (positive part / damage activation), `macauley_minus(e) = -min(e, 0)` (negative part / plastic dissipation), `heaviside(e) = ge(e, 0)` (right-continuous step using #136 comparison nodes), `smoothed_macauley(e, eps) = (e + sqrt(e² + eps²)) / 2` (C^∞ regularisation for Newton-friendly damage models). Construction-time folds: `macauley_plus(-x) → macauley_minus(x)` (via `scalar_negative` detection), `macauley_minus(-x) → macauley_plus(x)`, and the idempotence `<<x>+>+ = <x>+`. Closes #138.
- `numeric_less(scalar_number, scalar_number)` with `__int128`-based safe cross-multiplication for rational comparisons (long-double fallback on MSVC). Closes #142.
- CONTRACT NOTE comments documenting the bilateral dependency between the t2s pow simplifier and the tensor ÷ t2s div operator (cross-referenced at both sites).
- Lock-in tests for: cross-rank constant folding, commutative multiplication identity, deep-nesting collapse, zero-precedence in mul / div, pow-of-pow flattening, NaN / complex edge cases.
- Curated numerical-diff golden tests (`tests/NumericalDiffTest.h`) with a reusable harness header (`tests/NumericalDiffHelpers.h`) providing `EXPECT_DIFF_MATCHES(expr, var, x0)` and a 5-point variant. Complements the random fuzz tests by giving deterministic, named per-operator coverage and explicit smooth-side checks for non-smooth operators (max, min, macauley, abs, if_then_else) that the fuzz tester intentionally skips. 30 new tests across polynomial / trig / inverse trig / exp / log / sqrt / pow / abs / max / min / macauley / smoothed-macauley / if_then_else / multi-variable separability. Closes #85.
- `levi_civita_tensor` leaf node and `levi_civita(dim)` factory (`tensor_std.h`) for the permutation symbol ε_{i₁…i_N} in N dimensions, N ∈ {2, 3, 4}. Rank is intrinsically equal to dim — unlike `identity_tensor` which takes both. Full visitor coverage (printer, latex_printer, evaluator, rebuild_visitor, differentiation, contains_expression). The data-layer evaluator computes components from permutation parity rather than going through `tmech::levi_civita` because tmech's dim-3 formula uses the opposite sign convention from the standard (det(I)=+1) one. Dim-4 LC is constructible symbolically but the eval framework's MaxDim=3 ceiling means a bare evaluation throws — documented in the node's doxygen and locked in by test. Closes #34.

### Changed

- Visitor type lists consolidated to a single `tensor_visitor_typedef.h` driven by the `NUMSIM_CAS_TENSOR_NODE_LIST` macro.
- Replaced the rank-2-only `kronecker_delta` node with the general `identity_tensor` node. The two were doing the same work at rank 2 (every visitor delegated to a common helper); the only behavioural difference was the printer. Differentiation results are now consistent across paths — both `diff(A, A)` and `diff(trace(A), A)` produce `identity_tensor`. Added comprehensive Doxygen to `identity_tensor.h` and a new "Identity Tensor" section in `docs/tensor.md` documenting the rank-2 (Kronecker delta) and rank-2R (minor identity) forms, plus the `tmech::eye` vs `tmech::otimesu` footgun for rank ≥ 4. Closes #188.
- Renamed `basis_change_imp` → `permute_indices_wrapper`. The class permutes tensor indices (e.g. transpose is `{2,1}`) — it does not change basis. Closes #52.
- Renamed folder `include/numsim_cas/tensor/functions/` → `include/numsim_cas/tensor/wrappers/`. The files in that directory are AST-node wrapper classes, not free functions; the new name matches the `_wrapper` suffix convention used by the class names themselves. Closes #55.
- Renamed data-layer `tensor_data_basis_change` → `tensor_data_permute_indices` (file and class) plus the local variables `basis_change_lhs/rhs/temp` in `tensor_data_inner_product.h`. Completes the wrapper-layer rename from #52 — the data class is the runtime evaluator for the AST node `permute_indices_wrapper`, and now uses matching terminology. Closes #182.
- Two missing tensor `pow()` simplification rules: `pow(identity_tensor, n) → identity_tensor` (the rank-2 identity is its own n-th power) and `pow(inv(A), n) → inv(pow(A, n))` (pulls the inverse outside so the inner `pow(A, n)` can fold further and the evaluator only inverts once). Closes #96.
- Construction-time simplifier completions for the tensor-function family:
  - `det()`: added `det(identity_tensor) → 1`, `det(inv(A)) → 1/det(A)`, `det(trans(A)) → det(A)`, and `det(u ⊗ v) → 0` (for dim ≥ 2). Closes #70.
  - `inv()`: added `inv(α · A) → inv(A) / α` (recursive). Closes #71.
  - `trace()`: added `trace(identity_tensor) → dim` (the existing `kronecker_delta` rule didn't cover the general identity_tensor node that appears in differentiation results). Closes #72.

### Removed

- `kronecker_delta` node and its header `include/numsim_cas/tensor/kronecker_delta.h`. Use `make_expression<identity_tensor>(dim, 2)` instead. Closes #188.
- ~1,200 lines of dead / commented-out / orphan code. Tier A (entirely commented-out headers, 8): `expression_crtp.h`, `get_hash_scalar.h`, `is_symbol.h`, `nonlinear_solver_base.h`, `numsim_cas_variant.h`, `tensor/tensor_globals.h`, `tensor_to_scalar/operators/tensor_to_scalar_div.h`, `tensor_to_scalar/operators/tensor_to_scalar_sub.h`, plus the empty `src/numsim_cas/functions.cpp`. Tier B (orphan headers with active code but zero consumers, 7): `result_expression.h`, `tensor/tensor_constant.h`, `tensor/tensor_functions_fwd.h`, `tensor/scalar_tensor_op.h`, `tensor/simplifier/tensor_inner_product_simplifier.h` (360 lines), `tensor/operators/scalar/tensor_scalar_div.h`, `tensor/operators/tensor/tensor_sub.h`. Tier C (dead blocks inside live files): the commented-out `tensor_scalar_div` printer override in `tensor/visitors/tensor_printer.h`, six commented-out overrides in `tensor_to_scalar/visitors/tensor_to_scalar_printer.h` for nodes deleted in earlier cleanups, dead forward declarations (`expression_crtp`, `tensor_to_scalar_div`) and the unused doc-comment in `scalar/scalar_mul.h`. Also dropped four stale `#include` lines that referenced the Tier A files. Closes #185.
- Dead `tensor_to_scalar_with_tensor_div` AST node (declared but never integrated with the visitor pattern). Closes #149.
- Dead `tensor_type_defs.h` file (parallel-and-divergent visitor list).
- Commented-out `tensor_to_scalar_with_tensor_div` printer override.

### Fixed

- Rational comparison overflow in `numeric_less` / `operator<` for numerators near 2^63. Closes #142.
- Replaced the regression test for #142 with one that actually exercises int64 overflow (the original test's numerators were divisible by 3 and normalized away from the overflow path). Closes #170.
- `inv()` now rejects the zero tensor at construction (`inv(tensor_zero)` and the composite `inv(0 · A)` form) with a clear "singular" error instead of silently building a symbolic `inv(0)` node that would NaN/Inf during evaluation. Closes #187.
- `inv()` now rejects rank > 2 inputs at construction. Previously the function accepted them and built a symbolic `tensor_inv` node that would fail at the tmech evaluator (which is rank-2 only) or silently produce wrong results. The `identity_tensor` short-circuit still fires first, so the rank-4 minor identity remains self-inverse. Closes #192.
- `diff(tanh(x), x)` no longer throws `"duplicate child insertion"`. `tanh` was defined as `sinh(e) / cosh(e)`, which expands to a sum/difference of `exp(x)` and `exp(-x)`; the quotient-rule differentiation then tried to insert the same `exp(x)` child twice into a single `n_ary_add`, tripping the duplicate-child guard. Re-defined `tanh(x) = (exp(2x) - 1) / (exp(2x) + 1)` — algebraically identical, single `exp()` term, no duplicate fan-out. The derivative now evaluates to `sech²(x)` within `1e-12` across the sampled range. Closes #180.
- `dev()`, `sym()`, `vol()`, `skew()`, and `trans()` now reject non-rank-2 inputs at construction with a clear `"only rank-2 tensors are supported"` error. `permute_indices()` rejects size-mismatched index sequences with `"indices size (N) must equal tensor rank (M)"`. Previously these silently built symbolic nodes the evaluator could not handle. Mirrors the rank-2 gate added to `inv()` in #192. Closes #53.

### Documentation

- Documented (with lock-in tests) the deliberate non-IEEE behaviour of NaN under structural-identity comparison folding. Closes #143.
- Documented (with lock-in tests) the real-then-imag total order used for complex numbers in `numeric_less`. Closes #144.
- Audited visitor overload coverage in `scalar_differentiation`, `scalar_evaluator`, `tensor_differentiation`, `tensor_evaluator`, `tensor_printer`, `tensor_to_scalar_differentiation`, and `tensor_to_scalar_printer`. Closes #38, #42, #43, #44, #45, #76, #77, #39.

[Unreleased]: https://github.com/NumSim-Stack/numsim-cas/compare/main...HEAD
