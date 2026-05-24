# Changelog

All notable changes to NumSim-CAS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and from v0.1.0 onward the project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `tag_invoke(mul_fn, …)` for `tensor × tensor_to_scalar` (and the symmetric pair) with full visitor coverage and a dedicated simplifier handling nested-mul collapse and scalar-coefficient bubbling. Closes #145.
- `tag_invoke(div_fn, tensor, t2s)` routing through `lhs × pow(rhs, -1)` for `tensor ÷ tensor_to_scalar`. Closes #147.
- Scalar comparison nodes (`lt`, `gt`, `le`, `ge`, `eq`, `ne`) producing Real-valued indicators (1.0 / 0.0) for the damage-activation idiom and the upcoming `if_then_else`. Closes #136.
- `numeric_less(scalar_number, scalar_number)` with `__int128`-based safe cross-multiplication for rational comparisons (long-double fallback on MSVC). Closes #142.
- CONTRACT NOTE comments documenting the bilateral dependency between the t2s pow simplifier and the tensor ÷ t2s div operator (cross-referenced at both sites).
- Lock-in tests for: cross-rank constant folding, commutative multiplication identity, deep-nesting collapse, zero-precedence in mul / div, pow-of-pow flattening, NaN / complex edge cases.

### Changed

- Visitor type lists consolidated to a single `tensor_visitor_typedef.h` driven by the `NUMSIM_CAS_TENSOR_NODE_LIST` macro.
- Renamed `basis_change_imp` → `permute_indices_wrapper`. The class permutes tensor indices (e.g. transpose is `{2,1}`) — it does not change basis. Closes #52.
- Renamed folder `include/numsim_cas/tensor/functions/` → `include/numsim_cas/tensor/wrappers/`. The files in that directory are AST-node wrapper classes, not free functions; the new name matches the `_wrapper` suffix convention used by the class names themselves. Closes #55.

### Removed

- Dead `tensor_to_scalar_with_tensor_div` AST node (declared but never integrated with the visitor pattern). Closes #149.
- Dead `tensor_type_defs.h` file (parallel-and-divergent visitor list).
- Commented-out `tensor_to_scalar_with_tensor_div` printer override.

### Fixed

- Rational comparison overflow in `numeric_less` / `operator<` for numerators near 2^63. Closes #142.

### Documentation

- Documented (with lock-in tests) the deliberate non-IEEE behaviour of NaN under structural-identity comparison folding. Closes #143.
- Documented (with lock-in tests) the real-then-imag total order used for complex numbers in `numeric_less`. Closes #144.
- Audited visitor overload coverage in `scalar_differentiation`, `scalar_evaluator`, `tensor_differentiation`, `tensor_evaluator`, `tensor_printer`, `tensor_to_scalar_differentiation`, and `tensor_to_scalar_printer`. Closes #38, #42, #43, #44, #45, #76, #77, #39.

[Unreleased]: https://github.com/NumSim-Stack/numsim-cas/compare/main...HEAD
