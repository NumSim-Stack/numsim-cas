# numsim-cas: Comprehensive Critical Review

> **Date**: 2026-02-22
> **Branch**: `move_to_virtual`
> **Codebase Stats**: 192 headers, 38 source files, 22 test files, ~21,400 LOC (library), ~6,100 LOC (tests), 384 test cases (519 after typed-test expansion), 3 expression domains

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Review](#2-architecture-review)
3. [Core Layer Review](#3-core-layer-review)
4. [Scalar Domain Review](#4-scalar-domain-review)
5. [Tensor Domain Review](#5-tensor-domain-review)
6. [Tensor-to-Scalar Domain Review](#6-tensor-to-scalar-domain-review)
7. [Build System & CI Review](#7-build-system--ci-review)
8. [Test Suite Review](#8-test-suite-review)
9. [API & Usability Review](#9-api--usability-review)
10. [Performance Review](#10-performance-review)
11. [Complete Issue Catalog](#11-complete-issue-catalog)
12. [Enhancement Proposals](#12-enhancement-proposals)

---

## 1. Executive Summary

numsim-cas is a well-engineered C++23 computer algebra system targeting continuum mechanics. The library demonstrates strong design fundamentals -- the virtual visitor pattern, tag_invoke CPO operators, domain traits abstraction, and construction-time simplification form a coherent architecture. The projection tensor algebra (sym/dev/vol/skew) is a standout feature with sophisticated algebraic rules.

**Overall assessment**: Production-quality core with clear room for growth in simplification coverage, documentation, thread safety, and API ergonomics.

### Strengths

- Clean three-domain architecture with shared core infrastructure
- Projection tensor algebra with idempotence, orthogonality, and subspace rules
- Construction-time simplification catches algebraic identities early
- Domain traits pattern enables code reuse across scalar and tensor-to-scalar
- Comprehensive CI across GCC, Clang, MSVC in Debug and Release
- Expression DAG with shared_ptr prevents cycles and enables sharing

### Key Concerns

- Thread safety not addressed (lazy hash caching)
- ~~Several dead/commented-out code sections~~ [RESOLVED — compare_equal_visitor.h, scalar_div.h, symTM_*.h removed]
- ~~Some test files not wired into the build~~ [RESOLVED]
- ~~Missing simplification rules for common algebraic identities~~ [PARTIALLY RESOLVED — trace linearity, det scaling/multiplicativity, norm scaling, exp·exp, sin²+cos² added]
- No public API documentation (doc comments, Doxygen)
- No benchmarking infrastructure despite existing benchmark directory

---

## 2. Architecture Review

### 2.1 Three-Domain Design

| Domain | Nodes | Purpose | Base Class |
|--------|-------|---------|------------|
| Scalar | 21 | Symbolic scalar algebra | `scalar_expression` |
| Tensor | 17 | Symbolic tensor algebra | `tensor_expression` |
| Tensor-to-Scalar | 15 | Cross-domain operations (trace, det, norm, dot, exp, sqrt) | `tensor_to_scalar_expression` |

**Verdict**: The three-domain split is well-motivated and cleanly implemented. Each domain follows the same pattern (base class, node list macro, visitor typedefs, simplifiers), making the architecture predictable and learnable.

**Issue**: The domains are not fully symmetric in capabilities:
- Scalar domain has `symbol_type = scalar`, T2S has `symbol_type = void`
- Tensor domain has no `constant_type` or `one_type`
- This asymmetry makes the domain traits pattern less generic than it could be

### 2.2 Expression DAG

Expressions form a Directed Acyclic Graph via `shared_ptr`. This is correct for a CAS -- DAGs allow subexpression sharing without cycles.

**Issue**: No weak_ptr usage anywhere. For very large expression trees, this means:
- No ability to detect shared subexpressions without hash comparison
- Memory stays alive as long as any path references it (expected for DAGs, but no explicit lifecycle management)

### 2.3 Node Registration

The X-macro pattern (`NUMSIM_CAS_SCALAR_NODE_LIST`, etc.) registers all nodes for visitor generation. This is a proven C++ pattern, but:

**Issue**: Adding a new node requires editing the macro AND creating the node header AND adding visitor overloads. There's no compile-time check that all visitors handle all nodes (a missing overload silently falls through to the visitor base class's default, which may not be obvious).

### 2.4 Cross-Domain Coupling

Tensor-to-scalar nodes bridge tensor and scalar domains. The `tensor_to_scalar_scalar_wrapper` wraps scalars for use in T2S expressions. The `tensor_to_scalar_with_tensor_mul` handles mixed tensor-T2S products.

**Issue**: This coupling creates circular header dependency risks. The solution (out-of-line definitions in .cpp) works but is fragile -- reorganizing headers could reintroduce the cycle.

---

## 3. Core Layer Review

### 3.1 expression.h

**Good**:
- Clean virtual base with lazy hash caching
- `operator==` uses hash-first fast path, then deep comparison
- `operator<` enables consistent ordering for canonical forms

**Issues**:
- **Thread safety**: `m_hash_value` is lazily computed via `hash_value()` but has no synchronization. Multiple threads calling `hash_value()` simultaneously on a newly-created expression will race.
- **No `std::atomic`**: Even relaxed atomics would prevent torn reads
- **mutable m_hash_value**: The `const` method `hash_value()` mutates state, which is correct for caching but violates const-correctness expectations

### 3.2 expression_holder.h

**Good**:
- RAII wrapper with validity checking
- Compound assignment operators (`+=`, `*=`) for ergonomic use
- Unary negation via CPO dispatch

**Issues**:
- ~~`operator-()` is non-const (takes `expression_holder` by value), which means `auto neg = -expr;` moves `expr` if it's an rvalue. This is intentional but the behavior differs from mathematical expectation~~ [RESOLVED — redundant non-const overload removed]
- ~~No `operator bool()` for truthy checks -- must call `is_valid()` explicitly~~ [RESOLVED — `operator bool()` added]
- ~~No `swap()` member function~~ [RESOLVED — `swap()` added]

### 3.3 visitor_base.h

**Good**:
- Three visitor variants (returning, mutating, const) cover all use cases
- CRTP `visitable_impl` provides type-safe dispatch with `static get_id()`
- `accept()` methods for all three visitor types

**Issues**:
- **No default visitor**: If a visitor doesn't override `operator()` for a node type, it gets a pure virtual call at runtime (crash), not a compile-time error. A default handler that throws or asserts would be safer.
- **Visitor interface size**: Each new node adds a pure virtual to all visitors. With 51 total node types across domains, visitors have very wide interfaces.

### 3.4 Hash System

**Good**:
- Boost-style `hash_combine` with golden ratio constant
- N-ary tree sorts child hashes for commutativity
- Coefficient exclusion rule enables `3*x` and `5*x` to hash identically for tree merging

**Issues**:
- **Hash collision risk**: The coefficient exclusion means `3*x + 5*y` and `2*x + 7*y` produce different n-ary tree hashes (because children x,y have different hashes), but `3*x` and `x` hash the same. This is by design for the simplifier but could cause false positives in hash-map lookups if not carefully managed.
- ~~**stack_buf size 16**: The stack buffer in `n_ary_tree::update_hash_value()` assumes most add/mul trees have ≤16 children. This is reasonable but could be a problem for very wide sums (e.g., finite element assembly).~~ [RESOLVED — replaced with `std::vector`]
- **No hash seed**: Hash values start from 0, making small expressions predictable. This is fine for a CAS but would be a concern if hashes were used for security.

### 3.5 n_ary_tree.h

**Good**:
- Hash-map storage gives O(1) child lookup
- Separate coefficient avoids polluting the child map with numeric factors
- `push_back` asserts no duplicates, catching logic errors early

**Issues**:
- **O(n log n) hash update**: Every call to `update_hash_value()` sorts all child hashes. For incrementally built trees, this means O(n^2 log n) total cost. An incremental hash (XOR of child hashes) would be O(n) but lose ordering sensitivity.
- **No iterator invalidation guarantee**: The hash_map is rebuilt on every push_back. If client code holds iterators, they're invalidated silently.
- **Ordered map vs unordered map**: The name `hash_map` suggests unordered, but the implementation uses `expr_ordered_map` which is ordered. This naming is misleading.

### 3.6 n_ary_vector.h

**Good**:
- Vector-based for non-commutative operations
- Preserves insertion order (important for tensor products)

**Issues**:
- Significant code duplication with n_ary_tree. Both have identical coefficient handling, similar comparison operators, and similar push_back semantics. Could be unified with a storage policy template.

> **Note**: n_ary_tree `hash_map()` accessor has been renamed to `symbol_map()` for clarity.

### 3.7 Domain Traits

**Good**:
- Clean separation of domain-specific types
- `try_numeric()` enables generic numeric simplification
- `make_constant()` centralizes constant creation

**Issues**:
- **Inconsistent constant types**: Scalar's `constant_type` has `.value()`, T2S's wraps a scalar expression. The `if constexpr` guard works but is fragile -- adding a fourth domain with yet another constant representation would require updating all generic algorithms.
- **No concept checking**: Domain traits don't use C++20 concepts to enforce the required interface. A domain that forgets to define `add_type` would get a cryptic template error.

### 3.8 CPO Infrastructure (tag_invoke)

**Good**:
- Industry-standard CPO pattern for operator customization
- ADL-based dispatch enables domain-specific implementations
- Clean separation of operator syntax (`+`, `-`, `*`, `/`) from implementation

**Issues**:
- **Boilerplate**: Each new CPO requires a struct definition, tag_invoke concept check, and inline constexpr instance. A macro or base class template could reduce this.
- **No error messages**: When `tag_invoke` fails to find an overload, the error message is about "no matching function for call to tag_invoke" with template parameter dumps. A `static_assert` with a human-readable message would help.

### 3.9 Simplifier Algorithms

**Good**:
- Generic algorithms in core/ with domain-specific thin wrappers
- Multi-level dispatch (dispatcher -> visitor -> node-specific handler)
- Comprehensive handling of edge cases (0, 1, identity, annihilator)

**Issues**:
- **Explosion of visitor classes**: `simplifier_add.h` alone has 7 dispatcher classes (510 LOC). Each domain then creates thin wrappers, multiplying the class count. The total simplifier class count across all domains is ~30+.
- **No simplification rule registry**: Rules are hard-coded in visitor methods. Adding a new simplification (e.g., `log(exp(x))`) requires modifying existing classes rather than registering a rule.
- **No simplification ordering guarantee**: When multiple simplifications apply, the order depends on the visitor dispatch chain. This is deterministic but not documented.

### 3.10 Other Core Components

#### scalar_number.h
- `std::variant<int64_t, double, complex<double>>` -- clean type-erasing numeric wrapper
- **Issue**: No rational number support. `1/3` becomes `0.333...` as double, losing exactness.

#### assumptions.h
- Rich assumption system (positive, negative, integer, etc.)
- **Issue**: 401 LOC but limited integration -- only scalar domain uses assumptions for `abs()`, `sign()`, `sqrt()` simplification. Tensor assumptions are separate (`tensor_assume.h`).

#### evaluator_base.h
- `std::any` for symbol values -- type-erased but requires runtime cast
- **Issue**: No compile-time type safety. Setting a tensor variable with a scalar value compiles but fails at runtime.

#### ~~compare_equal_visitor.h~~ [RESOLVED — removed]
- ~~Entirely commented out -- dead code that should be removed~~

#### limit_result.h
- Growth rate classification (constant, logarithmic, polynomial, exponential)
- **Issue**: No integration with main simplification pipeline. Appears to be early-stage/experimental.

---

## 4. Scalar Domain Review

### 4.1 Node Types (21)

All node types are well-defined with consistent patterns:
- Constants: `scalar_zero`, `scalar_one`, `scalar_constant`
- Symbol: `scalar`
- Arithmetic: `scalar_add`, `scalar_mul`, `scalar_negative`, `scalar_pow`
- Functions: `scalar_sin`, `scalar_cos`, `scalar_tan`, `scalar_asin`, `scalar_acos`, `scalar_atan`, `scalar_sqrt`, `scalar_exp`, `scalar_log`, `scalar_abs`, `scalar_sign`
- Other: `scalar_named_expression`, `scalar_rational`

**Issues**:
- ~~**`scalar_div` is commented out** in the node list but `scalar_div.h` exists and is included in `numsim_cas.h`. Division is implemented as `x * pow(y, -1)`. The header file should either be removed or the div node re-enabled.~~ [RESOLVED — removed]
- **`scalar_rational`** exists but is rarely used. Its relationship to `scalar_pow` with negative exponents is unclear.
- **No `scalar_min`/`scalar_max`**: Common operations in optimization contexts are missing.

### 4.2 Operators

**Good**:
- Division correctly converts to `x * pow(y, -1)` for uniform handling
- Negation detects double negation: `-(-x)` -> `x`
- Zero/one special cases handled in operators before reaching simplifiers

**Issues**:
- ~~**`scalar_one` hash initialization**: Constructor initializes hash, while `scalar_zero` defers to `update_hash_value()`. Inconsistent pattern.~~ [RESOLVED — `scalar_one` now uses lazy hash like `scalar_zero`]

### 4.3 Simplifiers

**Well-handled cases**:
- `x + 0 -> x`, `x * 0 -> 0`, `x * 1 -> x`
- `x + x -> 2*x`, `x * x -> pow(x, 2)`
- `c1 + c2 -> c3` (constant folding)
- `(a+b) + c -> a+b+c` (flattening)
- `x + (-x) -> 0`
- `pow(pow(x, a), b) -> pow(x, a*b)`

**Missing simplifications**:
- `log(x*y) -> log(x) + log(y)` (not always valid, needs assumptions)
- `exp(x+y) -> exp(x)*exp(y)` (not always valid, needs assumptions)
- ~~`sin(x)^2 + cos(x)^2 -> 1`~~ [RESOLVED]
- ~~`exp(x)*exp(y) -> exp(x+y)`~~ [RESOLVED]
- `log(exp(x)) -> x` only at construction time, not when exp(x) arrives via simplification
- `x^a * x^b -> x^(a+b)` when a,b are non-integer (needs assumptions about x > 0)
- `pow(x*y, n) -> pow(x, n) * pow(y, n)` (needs assumptions)

### 4.4 Construction-Time Simplification

**Comprehensively handled in scalar_std.h**:
- `sin(0)->0`, `cos(0)->1`, `tan(0)->0`
- Inverse pairs: `sin(asin(x))->x`, etc.
- `exp(0)->1`, `log(1)->0`, inverse pairs
- `sqrt(0)->0`, `sqrt(1)->1`
- `abs(x)->x` when positive, `sign(x)->1` when positive

**Issues**:
- **No `sqrt(pow(x, 2)) -> abs(x)` rule** (only `sqrt(pow(x,2)) -> x` when x >= 0)
- **`sign(x)` derivative is 0**: Mathematically the derivative doesn't exist at 0, and is 0 elsewhere. Returning 0 is pragmatic but could be a Dirac delta in distribution theory.

### 4.5 Differentiation

**Well-implemented**: All standard rules (product, quotient, chain) with correct formulas for all function nodes.

**Issues**:
- **No higher-order differentiation** helper (must call `diff()` repeatedly)
- **No partial derivative notation** for printing
- **General power rule**: `d/dx(u^v) = u^(v-1)*(v'*log(u)*u + v*u')` -- correct but could lose precision when v is integer

### 4.6 Evaluator

**Good**: Template-based, supports any arithmetic type.

**Issue**: Uses `std::any_cast` for symbol lookup -- runtime type mismatch gives unhelpful errors.

### 4.7 Printer

**Good**: Precedence-aware, produces clean canonical forms.

**Issues**:
- **No LaTeX output** format
- **No MathML output** format
- **Power printed as `pow(x, 2)`** instead of `x^2` -- functional notation is unambiguous but less readable

---

## 5. Tensor Domain Review

### 5.1 Node Types (17)

Well-structured with clear separation of concerns:
- Leaf: `tensor`, `tensor_zero`, `identity_tensor`, `kronecker_delta`, `tensor_projector`
- Arithmetic: `tensor_add`, `tensor_mul`, `tensor_pow`, `tensor_negative`, `tensor_scalar_mul`
- Products: `inner_product_wrapper`, `outer_product_wrapper`, `basis_change_imp`, `simple_outer_product`
- Special: `tensor_inv`, `tensor_to_scalar_with_tensor_mul`

**Issues**:
- **`tensor_mul` uses `n_ary_vector`** (non-commutative) but tensor products are associative. The current implementation treats `A*B*C` as `((A*B)*C)` which is correct but the vector storage doesn't enforce associativity.
- **`simple_outer_product`**: Used internally by evaluator but also a node type. Its relationship to `outer_product_wrapper` could be confusing.
- **No `tensor_transpose`** node -- transpose is handled via `basis_change_imp` with indices `{2,1}`. This is mathematically correct but makes it harder to detect transposition in simplifiers.

### 5.2 Tensor Space System

**Excellent design**:
- Variant-based spaces: `{perm, trace}` with join semantics
- Propagation through operations (add joins spaces, neg preserves, etc.)
- Used by projector algebra for contraction/addition rules

**Issues**:
- **Young tableaux**: `Young` is in the variant but appears unused in current code
- **Space downgrade**: When spaces are incompatible, the join returns nullopt and the space is cleared silently. A warning or logging mechanism would help debugging.

### 5.3 Projection Tensor System

**Standout feature**: The P_sym, P_skew, P_vol, P_devi projector algebra with:
- Idempotence: `P:P -> P`
- Orthogonality: `P_sym:P_skew -> 0`
- Subspace: `P_vol:P_sym -> P_vol`
- Addition: `P_vol + P_devi -> P_sym`, `P_sym + P_skew -> I`
- Construction-time: `dev(dev(A)) -> dev(A)`, `vol(dev(A)) -> 0`

**Issues**:
- **Only rank-2 projectors**: Higher-rank projectors (rank 6, 8) not implemented
- **No projector composition**: `P_vol:A:P_sym` is handled as two separate contractions, not as a single composed projector
- **Evaluator short-circuits only rank-2**: Generic rank-4 fallback involves full tensor contraction, which is expensive

### 5.4 Inner Product Simplifier

**Handles**:
- Kronecker delta contractions
- Identity tensor contractions
- Outer product factorization
- Scalar multiplication extraction

**Missing**:
- **Associativity**: `(A:B):C` vs `A:(B:C)` not automatically reassociated
- **Trace extraction**: `I:A` could simplify to `trace(A)*I` for specific index patterns

### 5.5 Differentiation

**Well-implemented** with space-aware derivatives:
- `d(sym_tensor)/d(sym_tensor) -> P_sym` (not I)
- Chain rule through all operations
- `tensor_pow` derivative expanded into concrete sum

**Issues**:
- **`tensor_pow` derivative**: Expanded at differentiation time, not simplified afterward. Could produce large expressions for high powers.
- **No second-order tensor derivative** notation

### 5.6 Evaluator

**Good**: Template-based with tmech backend, handles all node types.

**Critical fix already applied**: `eye<T,D,4>` gives wrong identity; fixed to use `otimesu(I,I)`.

**Issues**:
- **Dimension/rank dispatch**: `tensor_data_eval` tries dimensions 1-3 and ranks 1-4 by default. Rank 5+ is not supported without changing template parameters.
- **No caching**: Evaluating the same subexpression multiple times recomputes it each time. CSE (Common Subexpression Elimination) at the evaluator level would improve performance.

---

## 6. Tensor-to-Scalar Domain Review

### 6.1 Node Types (15)

Clean bridge between tensor and scalar domains:
- Constants: `tensor_to_scalar_zero`, `tensor_to_scalar_one`
- Bridge: `tensor_to_scalar_scalar_wrapper`
- Tensor operations: `tensor_trace`, `tensor_dot`, `tensor_det`, `tensor_norm`
- Arithmetic: `tensor_to_scalar_add`, `tensor_to_scalar_mul`, `tensor_to_scalar_pow`, `tensor_to_scalar_negative`, `tensor_to_scalar_log`, `tensor_to_scalar_exp`, `tensor_to_scalar_sqrt`
- Product: `tensor_inner_product_to_scalar`

**Issues**:
- ~~**No `tensor_to_scalar_exp`**: `exp(trace(A))` cannot be represented purely in T2S~~ [RESOLVED]
- ~~**No `tensor_to_scalar_sqrt`**: `sqrt(det(A))` requires wrapping in scalar `sqrt`~~ [RESOLVED]
- **`symbol_type = void`**: Cannot create symbolic T2S variables directly. Must use scalar wrapper.

### 6.2 Construction-Time Simplifications

**Well-handled**:
- `trace(0) -> 0`, `trace(I) -> dim`, `trace(s*A) -> s*trace(A)`
- `det(0) -> 0`, `det(I) -> 1`
- `norm(0) -> 0`, `dot(0) -> 0`

**Missing**:
- ~~`trace(A + B) -> trace(A) + trace(B)` (linearity)~~ [RESOLVED]
- ~~`det(s*A) -> s^dim * det(A)` (determinant scaling)~~ [RESOLVED]
- ~~`det(A*B) -> det(A)*det(B)` (multiplicativity)~~ [RESOLVED]
- `trace(A*B) -> trace(B*A)` (cyclic property)
- ~~`norm(s*A) -> |s|*norm(A)` (norm scaling)~~ [RESOLVED]

### 6.3 Differentiation

Differentiating T2S with respect to tensors produces tensor expressions:
- `d(trace(A))/dA -> I`
- `d(det(A))/dA -> adj(A)` (cofactor matrix)
- `d(dot(A))/dA -> 2*A`

**Issues**:
- `d(norm(A))/dA` should be `A/norm(A)` but this requires handling division by T2S expressions in the tensor domain

---

## 7. Build System & CI Review

### 7.1 CMake

**Good**:
- Modern CMake (3.22+) with proper target-based configuration
- `GLOB_RECURSE` with `CONFIGURE_DEPENDS` for automatic source discovery
- FetchContent for tmech and GoogleTest
- Platform-specific flags for GCC/Clang/MSVC
- Sanitizer support (ASAN + UBSAN)
- Proper install targets with export

**Issues**:
- **`GLOB_RECURSE`**: CMake officially discourages GLOB for sources because new files require re-running cmake. `CONFIGURE_DEPENDS` mitigates but isn't supported by all generators.
- ~~**README option names wrong**: README says `BUILD_TESTS` but code uses `NUMSIM_CAS_BUILD_TESTS`~~ [RESOLVED]
- ~~**No `CMAKE_EXPORT_COMPILE_COMMANDS`**: Would help IDE integration (clangd, etc.)~~ [RESOLVED]
- **No `POSITION_INDEPENDENT_CODE`**: Required if library is linked into a shared library downstream
- **Static library only**: `add_library` without `SHARED`/`STATIC` defaults to the global `BUILD_SHARED_LIBS` which is usually static. No option to build shared.
- **tmech pinned to `master`**: Should pin to a specific commit/tag for reproducibility

### 7.2 CI (GitHub Actions)

**Good**:
- 8 build configurations: 4 compilers x 2 build types
- Runs on Ubuntu, macOS, Windows
- clang-format check in separate workflow

**Issues**:
- **No code coverage reporting** (lcov/gcov)
- **No static analysis** (clang-tidy, cppcheck, PVS-Studio)
- **No memory sanitizer** (MSAN) in CI
- **No thread sanitizer** (TSAN) in CI
- **No benchmark regression tracking**
- **Workflow triggers on all branches**: Could be noisy for WIP branches

### 7.3 Test CMake

**Issues**:
- **`-Werror` in tests**: Good for CI but can break during development when compiler is upgraded
- ~~**`-ftime-report`**: Enabled unconditionally in tests, producing timing output on every build. Should be behind an option.~~ [RESOLVED]
- ~~**Missing test files**: Several test headers exist but aren't listed in `target_sources`~~ [RESOLVED — all test headers now wired into build]

---

## 8. Test Suite Review

### 8.1 Coverage

| Test File | Domain | Approx Tests | Coverage |
|-----------|--------|-------------|----------|
| ScalarExpressionTest.h | Scalar | ~120 | Arithmetic, printing, canonicalization |
| ScalarDifferentiationTest.h | Scalar | ~40 | All derivative rules |
| ScalarEvaluatorTest.h | Scalar | ~20 | Numeric evaluation |
| ScalarAssumptionTest.h | Scalar | ~15 | Assumption inference |
| ScalarSubstitutionTest.h | Scalar | ~10 | Expression substitution |
| TensorExpressionTest.h | Tensor | ~50 | Arithmetic, products, projections |
| TensorDifferentiationTest.h | Tensor | ~30 | Tensor derivatives |
| TensorEvaluatorTest.h | Tensor | ~20 | Numeric evaluation with tmech |
| TensorProjectorDifferentiationTest.h | Tensor | ~15 | Projector derivatives |
| TensorSpacePropagationTest.h | Tensor | ~15 | Space tracking |
| TensorSubstitutionTest.h | Tensor | ~10 | Tensor substitution |
| TensorToScalarExpressionTest.h | T2S | ~20 | T2S arithmetic |
| TensorToScalarDifferentiationTest.h | T2S | ~10 | T2S derivatives |
| TensorToScalarEvaluatorTest.h | T2S | ~10 | T2S evaluation |
| TensorToScalarSubstitutionTest.h | T2S | ~5 | T2S substitution |
| CoreBugFixTest.h | Core | ~5 | Regression tests |
| LimitVisitorTest.h | Core | ~5 | Limit analysis |

### 8.2 Testing Strengths

- **EXPECT_PRINT** macro: Tests canonical printed form, catching both simplification and printing bugs
- **EXPECT_SAME_PRINT**: Tests commutativity by comparing printed forms of reordered expressions
- **Typed tests**: `TensorToScalarExpressionTest` runs across dimensions 1, 2, 3
- **Property-based**: Tests verify algebraic properties (associativity, commutativity, identity)

### 8.3 Testing Gaps

- **No fuzz testing**: Random expression generation could find edge cases
- **No performance tests**: No benchmarks for simplification or evaluation speed
- **No large expression tests**: All test expressions are small (≤10 nodes). Real-world continuum mechanics expressions can be much larger.
- ~~**Old test files entirely commented out**: `symTM_test.h`, `symTM_print_test.h`, `symTM_diff_test.h` are all commented out -- legacy tests from before the virtual visitor refactor~~ [RESOLVED — removed]
- **No negative tests**: No tests that verify expected failures (e.g., type mismatch, invalid operations)
- **No evaluator accuracy tests**: Evaluator tests check exact equality but real computations need tolerance checks for complex expressions
- **Missing cross-domain substitution tests**: Substitute scalar in tensor, tensor in T2S, etc.
- **No serialization/deserialization tests**: (Because no serialization exists yet)

---

## 9. API & Usability Review

### 9.1 Expression Construction

```cpp
auto [x, y] = make_scalar_variable("x", "y");
auto [A, B] = make_tensor_variable(
    std::tuple{"A", 3, 2}, std::tuple{"B", 3, 2});
```

**Good**: Structured bindings, variadic helpers, tuple-based tensor creation.

**Issues**:
- **`std::tuple` for tensor creation**: Positional arguments (name, dim, rank) are error-prone. A named-parameter approach or builder pattern would be safer.
- **No `make_tensor_constant`** helper for numeric tensors
- **Constants require explicit creation**: `make_scalar_constant(2)` is verbose compared to SymPy's `Integer(2)` or Mathematica's just `2`

### 9.2 Operator Overloads

**Good**: Natural syntax `x + y`, `A * B`, `pow(x, 2)`.

**Issues**:
- **`using std::pow`** required to avoid ambiguity -- the tests explicitly warn about this
- **`pow()` as only exponentiation syntax**: No `x^2` operator (C++ `^` is XOR). The `pow()` call is verbose for common cases.
- **Integer literals**: `x + 2` works (implicit conversion via `make_constant`), but `2 + x` also works, which is nice.

### 9.3 Printing

**Good**: `to_string()` and `operator<<` both available.

**Issues**:
- **No configurable print format** (infix vs prefix, LaTeX, etc.)
- **Pow always printed as function call**: `pow(x,2)` not `x^2` or `x**2`
- **No pretty-printing** for matrices or tensors (component-wise output)

### 9.4 Differentiation

```cpp
auto df = diff(f, x);        // scalar
auto dA = diff(expr, A);     // tensor
```

**Good**: Clean unified API across domains.

**Issues**:
- **No gradient/Jacobian/Hessian** convenience functions
- **No `diff(expr, x, 2)`** for higher-order derivatives
- **No automatic differentiation** (forward/reverse mode) -- only symbolic

### 9.5 Evaluation

```cpp
scalar_evaluator<double> ev;
ev.set(x, 3.0);
double result = ev.apply(f);
```

**Good**: Template-based for any numeric type.

**Issues**:
- **Evaluator is stateful**: Must `.set()` all variables before `.apply()`. No functional interface like `eval(f, {{x, 3.0}, {y, 2.0}})`.
- **No batch evaluation**: Evaluating the same expression at many points requires repeated `.set()` + `.apply()` calls.
- **No compiled evaluation**: Expression could be JIT-compiled or code-generated for performance.

### 9.6 Public API Surface

**Issue**: No public API header beyond `numsim_cas.h`. The umbrella header includes 55+ individual headers. Users must know internal structure to include specific features.

---

## 10. Performance Review

### 10.1 Memory

- **Shared_ptr overhead**: Every expression node has a `shared_ptr` wrapper. For small nodes (scalar_zero, scalar_one), the control block overhead (~16 bytes) is significant relative to the payload.
- **No small-buffer optimization**: Even trivial expressions allocate heap memory.
- **No expression pool/arena**: Frequent expression creation/destruction fragments the heap.

### 10.2 Computation

- **Simplification on every operation**: Every `+`, `-`, `*`, `/` triggers a full simplifier dispatch chain. For building large expressions incrementally, this adds up.
- **No lazy simplification mode**: Can't defer simplification until explicitly requested.
- **Hash recomputation**: n_ary_tree recomputes hash on every `push_back`. For building sums of N terms, this is O(N^2 log N).
- **No Common Subexpression Elimination**: Evaluator recomputes shared subexpressions.

### 10.3 Missing Benchmarks

The `benchmarks/` directory exists with a `poly_verse_variant` benchmark but:
- No results or data
- No automated benchmark tracking
- No comparison against other CAS libraries

---

## 11. Complete Issue Catalog

### 11.1 Critical Issues

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| C1 | Thread-unsafe lazy hash caching | `expression.h` | Data race in multithreaded use |
| ~~C2~~ | ~~Missing test files not compiled~~ | ~~`tests/CMakeLists.txt`~~ | ~~Tests exist but never run~~ [RESOLVED] |
| ~~C3~~ | ~~README option names incorrect~~ | ~~`README.md` line 98~~ | ~~Users will use wrong CMake options~~ [RESOLVED] |

### 11.2 Major Issues

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| ~~M1~~ | ~~`compare_equal_visitor.h` entirely commented out~~ | ~~`core/`~~ | ~~Dead code, confusing~~ [RESOLVED — removed] |
| ~~M2~~ | ~~`symTM_test.h` / `symTM_print_test.h` / `symTM_diff_test.h` entirely commented out~~ | ~~`tests/`~~ | ~~500 LOC of dead code~~ [RESOLVED — removed] |
| ~~M3~~ | ~~`scalar_div.h` included but `scalar_div` commented out in node list~~ | ~~`scalar/`~~ | ~~Inconsistent, confusing~~ [RESOLVED — removed] |
| M4 | No default handler in visitor base | `visitor_base.h` | Missing node handler = runtime crash |
| ~~M5~~ | ~~`-ftime-report` unconditional in test build~~ | ~~`tests/CMakeLists.txt`~~ | ~~Noisy build output~~ [RESOLVED] |
| M6 | tmech pinned to `master` branch | `CMakeLists.txt` line 45 | Non-reproducible builds |
| ~~M7~~ | ~~`n_ary_tree` hash_map naming misleading~~ | ~~`n_ary_tree.h`~~ | ~~Uses ordered map, named hash_map~~ [RESOLVED — renamed to `symbol_map()`] |

### 11.3 Minor Issues

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| ~~m1~~ | ~~`scalar_one` vs `scalar_zero` hash initialization inconsistency~~ | ~~`scalar_one.h`, `scalar_zero.h`~~ | ~~Style inconsistency~~ [RESOLVED] |
| ~~m2~~ | ~~No `operator bool()` on `expression_holder`~~ | ~~`expression_holder.h`~~ | ~~Must use `is_valid()`~~ [RESOLVED] |
| m3 | `scalar_rational` node exists but is rarely used | `scalar/` | Symbolic numerator/denominator node; division currently implemented as `x * pow(y, -1)` instead |
| ~~m4~~ | ~~No `swap()` on `expression_holder`~~ | ~~`expression_holder.h`~~ | ~~Missing standard utility~~ [RESOLVED] |
| ~~m5~~ | ~~No `CMAKE_EXPORT_COMPILE_COMMANDS`~~ | ~~`CMakeLists.txt`~~ | ~~Poor IDE integration~~ [RESOLVED] |
| m6 | Young tableau space variant unused | `tensor_space.h` | Dead code path |
| m7 | `tensor_to_scalar_with_tensor_mul` breaks header-only promise | `tensor/` | Requires .cpp for cross-domain |
| ~~m8~~ | ~~Stack buffer size 16 hardcoded in n_ary_tree~~ | ~~`n_ary_tree.h`~~ | ~~Potential heap fallback for wide trees~~ [RESOLVED — replaced with std::vector] |

### 11.4 Code Smells

| # | Smell | Location | Description |
|---|-------|----------|-------------|
| S1 | Duplication between `n_ary_tree` and `n_ary_vector` | `core/` | ~100 LOC of identical coefficient/comparison logic (note: `hash_map` renamed to `symbol_map`) |
| S2 | Simplifier class explosion | `simplifier/` | 7+ dispatcher classes per operation |
| S3 | `std::any` in evaluator_base | `evaluator_base.h` | Runtime type checking, no compile-time safety |
| S4 | CPO boilerplate | `binary_ops.h`, etc. | Repetitive struct definitions |
| S5 | Mixed `if constexpr` and runtime dispatch | Across domains | Inconsistent dispatch patterns |
| S6 | No logging/tracing infrastructure | Everywhere | Difficult to debug simplification |

---

## 12. Enhancement Proposals

### 12.1 High Priority

#### E1: Thread-Safe Hash Caching
Replace `mutable std::size_t m_hash_value` with `mutable std::atomic<std::size_t>` using relaxed ordering. Cost is minimal; benefit is thread safety.

#### ~~E2: Fix Test Build~~ [RESOLVED]
~~Wire all test headers into `tests/CMakeLists.txt`.~~ All test headers now compiled and running.

#### ~~E3: Remove Dead Code~~ [RESOLVED]
~~Delete `compare_equal_visitor.h`, `symTM_test.h`, `symTM_print_test.h`, `symTM_diff_test.h`, `scalar_div.h`.~~ All removed.

#### E4: Add Rational Number Type
Extend `scalar_number` with an exact rational type (`std::pair<int64_t, int64_t>` or a proper rational class). This would enable:
- `1/3 + 1/3 = 2/3` (exact)
- `pow(x, 1/2)` as exact half-power
- Cleaner symbolic coefficient handling

> **Note**: A `scalar_rational` *node* already exists as a symbolic `binary_op` (numerator/denominator), with printer, evaluator, limit, and assumption visitor support. However, the division operator currently bypasses it (`x / y` → `x * pow(y, -1)`), and `scalar_number` itself has no rational variant — only `int64_t | double | complex<double>`. E4 is about adding exact rational arithmetic at the `scalar_number` level and routing division through `scalar_rational` where appropriate, not creating a new node type.

### 12.2 Medium Priority

#### E5: LaTeX Printer
Add a `latex_printer` visitor that outputs:
- `\frac{a}{b}` for division
- `x^{2}` for powers
- `\sin`, `\cos`, etc. for functions
- `\mathbf{A}` for tensors
- `\text{tr}(\mathbf{A})` for trace

#### E6: Additional Simplification Rules
```
// Logarithmic
log(exp(x)) -> x                    (already at construction)
log(x*y) -> log(x) + log(y)         (requires x > 0, y > 0)
log(x^n) -> n*log(x)                (requires x > 0)
log(1/x) -> -log(x)                 (requires x > 0)

// Exponential
exp(log(x)) -> x                    (already at construction)
exp(a) * exp(b) -> exp(a+b)         [RESOLVED]
exp(a)^n -> exp(n*a)                (always valid)

// Trigonometric
sin(x)^2 + cos(x)^2 -> 1           [RESOLVED]
sin(-x) -> -sin(x)                  (odd function)
cos(-x) -> cos(x)                   (even function)
sin(pi - x) -> sin(x)
cos(pi/2 - x) -> sin(x)

// Tensor-to-scalar
trace(A + B) -> trace(A) + trace(B)     [RESOLVED]
det(s*A) -> s^dim * det(A)              [RESOLVED]
det(A*B) -> det(A)*det(B)               [RESOLVED]
trace(A*B) = trace(B*A)                 (cyclic property)
```

#### E7: Expression Serialization
Add save/load capability for expression trees (JSON, binary, or S-expression format). This enables:
- Caching expensive symbolic computations
- Transmitting expressions between processes
- Debugging by inspecting serialized trees

#### E8: Common Subexpression Elimination (CSE) in Evaluator
Before evaluating, walk the expression tree and identify shared subexpressions. Evaluate each unique subtree once, store the result, and reuse it. This is standard in code generation from CAS systems.

#### E9: Concept-Checked Domain Traits
```cpp
template <typename Domain>
concept ExpressionDomain = requires {
    typename domain_traits<Domain>::expr_holder_t;
    typename domain_traits<Domain>::add_type;
    typename domain_traits<Domain>::mul_type;
    typename domain_traits<Domain>::zero_type;
    { domain_traits<Domain>::zero() } -> std::same_as<typename domain_traits<Domain>::expr_holder_t>;
};
```

#### E10: Static Analysis in CI
Add clang-tidy with a `.clang-tidy` configuration to catch:
- Unused variables/includes
- Missing `const` qualifiers
- Pointer arithmetic issues
- Modern C++ idiom suggestions

### 12.3 Lower Priority

#### E11: Code Generation Backend
Generate C/C++/CUDA code from simplified expressions for high-performance numerical evaluation. This is the standard path for production CAS use in simulation.

#### E12: Pattern Matching Engine
Replace hard-coded simplifier visitor methods with a declarative rule system:
```cpp
register_rule(pow(sin(X_), 2) + pow(cos(X_), 2), one());
register_rule(exp(log(X_)), X_);
register_rule(log(exp(X_)), X_);
```
This would make adding new simplification rules trivial.

#### E13: Expression Complexity Metric
Add a `complexity(expr)` function that counts nodes, depth, and operation types. Useful for:
- Choosing between equivalent forms (simplify to minimum complexity)
- Performance estimation
- Progress tracking during simplification

#### E14: Lazy/Deferred Simplification Mode
Allow building expressions without immediate simplification, then simplify on demand:
```cpp
auto f = build_raw(x + y + x);   // stores unsimplified
auto g = simplify(f);             // applies all rules
```
This would improve performance for expression construction in tight loops.

#### E15: Parallel Simplification
For large expressions with independent subtrees, simplify subtrees in parallel using a thread pool. The immutable DAG structure makes this naturally safe (once hash caching is thread-safe).

#### E16: Expression Diff/Patch
Given two expressions, compute a structural diff showing what changed. Useful for debugging simplification and for version-control-like expression tracking.

#### E17: Interactive Simplification Stepper
A debug tool that shows each simplification step applied:
```
Input: (x + y) + (x - y)
Step 1: flatten add -> x + y + x + (-y)
Step 2: combine x + x -> 2*x
Step 3: combine y + (-y) -> 0
Step 4: remove 0 -> 2*x
Result: 2*x
```

#### E18: Support for Piecewise Functions
Add a `piecewise(condition, expr_true, expr_false)` node for conditional expressions. Needed for real-world mechanics (yield conditions, contact, etc.).

#### E19: Matrix-Level Operations
Add `eigenvalues(A)`, `eigenvectors(A)`, `svd(A)` as symbolic operations. These produce vectors/matrices of scalar eigenvalues.

#### E20: Assumption Propagation
Extend the assumption system so that:
- `assume_positive(x); assume_positive(y);` implies `x*y` is positive
- `assume_symmetric(A);` implies `trace(A*B) = trace(B*A)` always holds
- Assumptions propagate through operations automatically

#### E21: C Code Generation for Finite Elements
Generate optimized C code for evaluating material tangent tensors. This is the primary use case for CAS in continuum mechanics -- derive the tangent symbolically, simplify, then generate code for the finite element solver.

#### E22: API Documentation (Doxygen)
Add doc-comments to all public API classes and functions. Generate HTML documentation with Doxygen. The existing `docs/` markdown files are a good start but lack API-level detail.

#### E23: Expression Hashing Quality Metrics
Add a test that generates many random expressions and measures hash collision rates. This would validate the hash function quality and detect regressions.

---

## Summary

numsim-cas is a well-architected CAS library with a solid foundation. The three-domain design, projection tensor algebra, and domain traits pattern are notable strengths. Since the initial review, significant progress has been made: all test files are now wired into the build (C2, E2), dead code has been removed (M1-M3, E3), README corrected (C3), `expression_holder` API improved (m1, m2, m4), n_ary_tree stack buffer and naming issues fixed (m8, M7), build system improved (m5, M5), and many simplification rules added (exp·exp, sin²+cos², trace linearity, det scaling/multiplicativity, norm scaling). The T2S domain was expanded with `tensor_to_scalar_exp` and `tensor_to_scalar_sqrt` nodes. The remaining areas for improvement are: thread safety (C1), expanded simplification rules, and developer-facing documentation. The enhancement proposals range from the remaining quick fix (E1) to medium-term improvements (E5-E10) that would significantly improve usability, to longer-term features (E11-E23) that would make the library competitive with established CAS systems for continuum mechanics applications.
