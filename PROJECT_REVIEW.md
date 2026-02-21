# NumSim-CAS Project Review

**Date**: 2026-02-20
**Branch**: `move_to_virtual` (55 commits ahead of `main`, 185 total)
**Status**: 480/480 tests passing, zero warnings

---

## 1. Executive Summary

NumSim-CAS is a C++23 header-only Computer Algebra System for symbolic tensor calculus. It supports three expression domains (scalar, tensor, tensor-to-scalar), automatic differentiation, algebraic simplification, numerical evaluation, and a projection tensor algebra system. The codebase totals **~32,000 lines** across 269 files with 572 class/struct definitions and 480 tests.

The architecture is well-designed with clear domain separation, consistent patterns, and solid mathematical foundations. The `move_to_virtual` branch represents a major refactoring from a previous `std::variant`-based architecture to a virtual visitor pattern, bringing improved extensibility and cleaner code organization.

**Overall Rating: 8/10** — Production-quality architecture with good test coverage. Needs documentation, minor cleanup, and a few targeted improvements.

---

## 2. Codebase Metrics

| Metric | Value |
|--------|-------|
| Total lines (headers + sources) | 31,869 |
| Header files (.h) | 216 (16,987 lines) |
| Source files (.cpp) | 53 (4,820 lines) |
| Test files | 19 (5,988 lines) |
| Classes / Structs | 410 / 162 (572 total) |
| Test cases | 480 (317 TEST/TEST_F macros, expanded by TYPED_TEST) |
| Examples | 5 (scalar basics, differentiation, evaluation, tensor, t2s) |
| Git commits (branch) | 185 |
| C++ standard | C++23 |
| Build time (tests) | ~2.3s |

### Largest Files

| Lines | File | Purpose |
|-------|------|---------|
| 967 | `tests/TensorEvaluatorTest.h` | Tensor evaluation tests |
| 672 | `scalar_assumption_propagator.cpp` | Assumption propagation |
| 619 | `tests/TensorExpressionTest.h` | Tensor expression tests |
| 553 | `tensor_printer.h` | Tensor output visitor |
| 536 | `tests/TensorToScalarExpressionTest.h` | T2S expression tests |
| 514 | `simplifier_add.h` | Generic add simplifier |
| 461 | `tensor_evaluator.h` | Tensor evaluator visitor |
| 424 | `simplifier_sub.h` | Generic sub simplifier |
| 420 | `scalar_differentiation.h` | Scalar differentiation |
| 401 | `assumptions.h` | Assumption system |

---

## 3. Architecture

### 3.1 Domain Model

Three parallel expression domains, each with identical structure:

```
                    expression (base)
                   /        |        \
     scalar_expression  tensor_expression  tensor_to_scalar_expression
          |                  |                       |
     scalar nodes       tensor nodes           t2s nodes
     scalar visitors    tensor visitors        t2s visitors
     scalar simplifiers tensor simplifiers     t2s simplifiers
```

Cross-domain bridges:
- `tensor_scalar_mul`: scalar x tensor -> tensor
- `tensor_to_scalar_scalar_wrapper`: scalar -> t2s
- `tensor_to_scalar_with_tensor_mul`: t2s x tensor -> tensor
- `tensor_inner_product_to_scalar`: tensor x tensor -> t2s

### 3.2 Core Design Patterns

| Pattern | Usage | Quality |
|---------|-------|---------|
| **Virtual visitor** | Expression dispatch (not std::variant) | Excellent — extensible, clean |
| **CRTP** | Simplifier thin wrappers, evaluator dispatch | Good — avoids virtual overhead in hot paths |
| **Tag-invoke CPO** | Operator overloading (add_fn, mul_fn, etc.) | Excellent — ADL-safe, extensible |
| **Expression holder** | RAII wrapper around `shared_ptr<node>` | Good — safe, ergonomic |
| **N-ary tree** | Commutative add/mul with coefficient | Excellent — hash-map children, O(1) lookup |
| **Domain traits** | Generic algorithms parameterized by domain | Excellent — eliminates duplication |
| **Hash-based identity** | Expression equality and ordering | Good — fast comparison, deterministic print |

### 3.3 Expression Node Counts

| Domain | Leaf | Unary | Binary | N-ary | Total |
|--------|------|-------|--------|-------|-------|
| Scalar | 4 | 13 | 1 | 2 | 20 |
| Tensor | 5 | 2 | 5 | 2 | 14+ |
| T2S | 3 | 6 | 2 | 2 | 13 |

### 3.4 Key Subsystems

**Simplification Engine**
- Construction-time simplification (operators create simplified expressions)
- Two-level design: generic algorithms in `core/simplifier/` + thin domain wrappers
- Rules: identity laws, annihilation, constant folding, like-term merging, power rules
- Projector algebra: idempotence (P:P=P), orthogonality, subspace relations, addition

**Differentiation**
- Visitor-based with Leibniz/chain rule
- Tensor: `dA/dA` returns identity (or projector with space assumptions)
- T2S: product/quotient/chain rules for scalar-valued tensor functions
- Cross-domain: `d(tr(A*B))/dA = B^T` via automatic chain rule

**Evaluation**
- Template-based numerical evaluation over `ValueType`
- Integrates with `tmech` library for tensor arithmetic
- Projector short-circuit: `P_sym:A` -> `tmech::sym(A)` directly

**Projection Tensor System**
- `dev()`, `sym()`, `vol()`, `skew()` as `inner_product(P, {3,4}, A, {1,2})`
- Space propagation through derived expressions
- Algebra rules at construction time: `dev(dev(A))=dev(A)`, `vol(A)+dev(A)=sym(A)`
- `tensor_space` = {permutation variant, trace variant} on tensor nodes

---

## 4. Strengths

### 4.1 Architecture
- **Clean domain separation**: Three domains with identical patterns; adding a new domain is straightforward
- **Generic core algorithms**: `simplifier_add.h`, `simplifier_sub.h`, etc. are parameterized by domain traits, eliminating ~60% of duplication
- **Hash-based identity**: Coefficient-excluding hashes enable `2*x + 3*x -> 5*x` without expensive structural comparison
- **Projector algebra**: Elegant mathematical design with `ProjKind` classification, contraction rules, and addition rules
- **No circular dependencies**: Well-layered include graph with clear direction

### 4.2 Code Quality
- **Consistent patterns**: Every domain follows the same structure — once you understand scalar, you understand all three
- **Move semantics**: Proper use of `std::move` and `std::forward` throughout
- **Const-correctness**: `[[nodiscard]]`, `const` references, deleted assignment operators
- **C++23 features**: `std::unreachable()`, concepts, `if constexpr`, structured bindings
- **Header-only design**: Enables aggressive inlining and template specialization

### 4.3 Testing
- **480 tests, all passing**: Comprehensive coverage across all domains
- **Parametric dimension tests**: TYPED_TEST with Dim={1,2,3} catches dimension-specific bugs
- **Expression string testing**: `EXPECT_PRINT` macro validates canonical forms
- **Evaluation accuracy**: Numerical tests with 1e-12 tolerance
- **Edge cases tested**: Zero handling, identity laws, hash invariants, exception safety

### 4.4 Mathematical Correctness
- **Space propagation**: Correctly handles non-preservation (pow/inv lose trace-freeness for deviatoric)
- **Identity tensor**: Uses `otimesu(I,I)` for rank-4 (not `eye<T,D,4>` which gives wrong pairing)
- **Projector orthogonality**: `dev(skew(A))=0`, `vol(skew(A))=0` etc.
- **Kronecker delta special cases**: `sym(I)=I`, `dev(I)=0`, `skew(I)=0`, `vol(I)=I`

---

## 5. Issues and Recommendations

### 5.1 Fixed

The following issues were identified and resolved:

- **H1. README**: Replaced single-line stub with comprehensive README covering quickstart, tensor examples, build instructions, and architecture overview
- **H2. CI/CD**: Updated `.github/workflows/cmake-multi-platform.yml` -- fixed branch name (`master` -> `main`), C++ standard (17/20 -> 23), compiler versions (GCC 14, Clang 18), added `--output-on-failure` to ctest
- **H3. Dead compare_less_visitors**: All 4 compare_less_visitor files (`scalar`, `tensor`, `tensor_to_scalar`, `core`) were dead code referencing nonexistent `compare_types_base_imp` class. Deleted them and removed includes from `numsim_cas.h` and `scalar_all.h`
- **M1. expression_holder overloads**: Consolidated 6 compound assignment overloads (3x `+=`, 3x `*=`) into 2 template versions using universal references with `requires` constraint
- **M5. Thread-safety documentation**: Already documented at `expression.h:80-82`
- **L1. Dead code**: Removed `get_first()` from `basic_functions.h`, commented-out includes from `tensor_to_scalar_definitions.h`, commented-out declarations from `tensor_to_scalar_functions.h`, commented-out code from `tensor_to_scalar_expression.h`
- **L2. Filename typo**: Renamed `tensor_to_scalar_simpilifier_mul.cpp` -> `tensor_to_scalar_simplifier_mul.cpp`
- **L3. binary_op::operator< redundancy**: Removed redundant equality check before per-operand comparison
- **L4. operator<< for t2s**: Already implemented in `tensor_to_scalar_io.h/.cpp` and included via `numsim_cas.h`

### 5.2 Remaining (Future Work)

**M2. Large simplifier headers**
`simplifier_add.h` (514 lines) and `simplifier_sub.h` (424 lines) define multiple nested classes inline. Consider splitting into separate files per dispatch class.

**M3. Macro-heavy visitor dispatch**
The `NUMSIM_LOOP_OVER` / `NUMSIM_CAS_*_NODE_LIST` macro pattern generates visitor overrides. A CRTP-based alternative could provide better IDE navigation and debugging support.

**M4. N-ary tree hash recomputation**
Every `push_back()` triggers a full hash recomputation. In practice, push_back is called 2-3 times (bulk merge operations copy the hash_map directly), so this is not a real bottleneck.

**H4. Pre-existing TensorToScalarExpressionTest failures on `main`**
Six ordering/division issues exist on the `main` branch. They are fixed on `move_to_virtual` and should be resolved upon merge.

**L5. Copy constructor boilerplate**
Many node classes have explicit copy/move constructors that only forward to base. Low risk/reward for refactoring.

---

## 6. Test Coverage Analysis

### Coverage by Feature

| Feature | Tests | Coverage |
|---------|-------|----------|
| Scalar expressions | 20 | Good — covers add/mul/pow simplification, printing |
| Scalar evaluation | 39 | Excellent — trig, exp, log, nested chains |
| Scalar assumptions | 27 | Excellent — propagation, construction-time simplification |
| Scalar differentiation | 5 | Basic — could use more chain rule / nested cases |
| Tensor expressions | 34 | Good — parametric over Dim={1,2,3} |
| Tensor evaluation | 56 | Excellent — inner products, projectors, identity |
| Tensor differentiation | 8 | Good — self, scalar_mul, pow |
| Tensor projector diff | 6 | Good — dev/sym/vol/skew |
| Tensor space propagation | 47 | Excellent — all operations, cross-domain |
| T2S expressions | 15 | Adequate — parametric, basic operations |
| T2S evaluation | 24 | Good — complex expressions |
| T2S differentiation | 11 | Good — product/chain rules |
| Core bug fixes | 20 | Excellent — regression tests for fixed bugs |
| Substitution | 18 | Good — all three domains |
| Limit analysis | 45 | Excellent — dependency, growth rates |

### Gaps

- **Inner products**: Only ~3 explicit inner product simplification tests
- **Rank-4 tensors**: Most tensor tests use rank-2; rank-4 behavior is underexplored
- **Performance/stress tests**: No tests for large expression trees (1000+ nodes)
- **Numerical edge cases**: NaN/Inf propagation, division by zero behavior
- **Serialization/deserialization**: No persistence tests
- **Property-based testing**: All tests use concrete examples; fuzz/property testing could catch edge cases

---

## 7. Build System

### Configuration

```cmake
cmake_minimum_required(VERSION 3.22)
project(NumSim_CAS LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
```

- **Dependencies**: `tmech` (FetchContent from GitHub), GoogleTest (FetchContent)
- **Source discovery**: `GLOB_RECURSE` on `src/*.cpp` — requires `cmake -B` reconfigure for new files
- **Warnings**: `-Wall -Wextra -Wpedantic` (GCC/Clang), `/W4` (MSVC), `-Werror` on tests
- **Sanitizers**: Optional ASAN/UBSAN via `NUMSIM_CAS_SANITIZERS=ON`
- **Install**: CMake export targets with version-less config

### Recommendations
- Add `cmake-format` / `cmake-lint` to enforce CMake style
- Consider `file(GLOB ...)` with `CONFIGURE_DEPENDS` as alternative to pure GLOB_RECURSE
- Add a `clang-format` target for automated formatting
- Pre-commit hooks for format checking

---

## 8. Dependency Analysis

| Dependency | Purpose | Coupling |
|------------|---------|----------|
| **tmech** | Tensor evaluation (numerical) | Medium — used only in evaluator visitors |
| **GoogleTest** | Testing | Test-only |
| **C++ stdlib** | Core | Standard |

The `tmech` dependency is well-isolated: only evaluator visitors and a few data classes reference it. The rest of the CAS operates purely symbolically with no external dependencies. This is good design — the symbolic engine is testable and usable without `tmech`.

---

## 9. Design Decisions Worth Noting

### Virtual Visitor vs. std::variant
The `move_to_virtual` branch migrated from `std::variant` to virtual visitors. This was the right call for this project: with 47+ node types across 3 domains, `std::variant` would require massive `std::visit` instantiations and make adding new nodes painful. The virtual visitor pattern trades a small runtime cost (vtable dispatch) for much better extensibility and compile times.

### Hash Design: Coefficient Exclusion
The rule that `scalar_mul(const, T).hash == T.hash` is critical for the n-ary tree to work correctly. It allows `2*x` and `3*x` to be stored under the same hash key in an add node, enabling automatic like-term merging. This is elegant but requires discipline — every new node type must respect the convention.

### Construction-Time Simplification
All operators simplify at construction time (not lazily). This means `a + b` immediately produces a simplified result. The upside is that all expressions are always in canonical form. The downside is that construction can trigger deep visitor dispatch chains. For this project's use case (symbolic differentiation of tensor expressions), this is the right trade-off.

### Projector Algebra as Inner Products
Replacing function nodes (`tensor_deviatoric`, `tensor_symmetry`, etc.) with `inner_product(P, {3,4}, A, {1,2})` was a significant design win. It unifies all projection operations under a single mechanism, enables algebraic simplification via projector algebra rules, and makes the system extensible to new projectors without new node types.

---

## 10. Roadmap Suggestions

1. **Documentation**: Write a proper README with quickstart, build instructions, and API examples
2. **CI/CD**: GitHub Actions for GCC/Clang/MSVC on Linux/macOS/Windows
3. **Merge to main**: The `move_to_virtual` branch has 55 commits of improvements; consider merging after addressing H3/H4
4. **Python bindings**: The `numsim-pycas` project exists separately; tighter integration or documentation of the binding layer would help
5. **Performance profiling**: Add benchmarks for large expression tree construction and simplification
6. **Rank-4 tensor tests**: Expand test coverage for higher-rank operations
7. **Code formatting**: Adopt a `.clang-format` configuration and enforce it project-wide

---

## 11. Conclusion

NumSim-CAS is a well-engineered symbolic computation library with a clean architecture, consistent patterns, and strong mathematical foundations. The `move_to_virtual` branch has significantly improved code quality through the domain traits pattern, projection tensor system, and space propagation. The 480 passing tests provide good confidence in correctness.

The main areas for improvement are documentation (README, API docs), CI/CD infrastructure, and minor code cleanup (dead code, filename typo, incomplete t2s comparison visitor). The core architecture and mathematical design are solid and ready for production use.
