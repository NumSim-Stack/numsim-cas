# Critical Review of numsim::cas

This document catalogs issues found in the library, organized by category and severity.
All file paths are relative to the project root.

---

## 1. Core Architecture Issues

### ~~1.1 Hash-Only Equality in `symbol_base`~~ — FIXED

**Fixed in:** `include/numsim_cas/core/symbol_base.h:66-81`

`operator==` and `operator<` now compare by `name()` instead of `hash_value()`,
eliminating hash collision risks. Tests in `CoreBugFixTest.h`.

---

### ~~1.2 `n_ary_tree::insert_hash()` Asserts Then Overwrites~~ — FIXED

**Fixed in:** `include/numsim_cas/core/n_ary_tree.h:151-155`

`assert` replaced with `throw internal_error(...)`. `noexcept` removed from
`insert_hash` and `push_back`. Duplicates now throw a catchable `internal_error`
in both debug and release builds. Tests in `CoreBugFixTest.h`.

---

### ~~1.3 `expression_holder::operator<` Inconsistent With `expression::operator==`~~ — FIXED

**Fixed in:** `include/numsim_cas/core/expression_holder.h`

After hash+id match, `operator<` now deep-compares via `expression::operator==`.
If equal -> returns false. On hash collision -> uses `std::less<ExprBase const*>`
address-based tiebreaker for deterministic total order. Tests in `CoreBugFixTest.h`.

---

### ~~1.4 `n_ary_tree` Dead Code: `add_child()` Never Defined~~ — FIXED

**Fixed in:** `include/numsim_cas/core/n_ary_tree.h`, `include/numsim_cas/core/n_ary_vector.h`

Deleted `init_parameter_pack` methods from both `n_ary_tree` and `n_ary_vector`.
They were private, never called, and referenced undefined `add_child()`.

---

### ~~1.5 Mutable Hash Caching Is Not Thread-Safe~~ — FIXED (documented)

**Fixed in:** `include/numsim_cas/core/expression.h`

Added comment on `m_hash_value` documenting the single-threaded assumption.
No code change needed — no threading exists in the codebase.

---

### ~~1.6 Unsafe `static_cast` in `visitor_base::equals_same_type`~~ — FIXED

**Fixed in:** `include/numsim_cas/core/visitor_base.h`

Added `dynamic_cast` assert before the `static_cast`. Zero overhead in release,
catches type mismatches in debug builds.

---

## 2. Const-Correctness

### ~~2.1 `coeff()` Const Method Returns Non-Const Reference~~ — FIXED

**Fixed in:** `include/numsim_cas/core/n_ary_vector.h:62`, `include/numsim_cas/core/n_ary_tree.h:75`

Changed `auto &coeff() const` to `auto const &coeff() const` in both files.
Compile-time `static_assert` test in `CoreBugFixTest.h`.

---

## 3. Null Safety

### ~~3.1 `expression_holder` Dereferences Without Validity Checks~~ — FIXED

**Fixed in:** `include/numsim_cas/core/expression_holder.h`

Added `throw_if_invalid()` guard to `operator*`, `operator->`, `get()`, `operator<`,
and `operator==`. Null access now throws `invalid_expression_error` instead of
crashing. Tests in `CoreBugFixTest.h`.

---

### ~~3.2 Unsafe `.back()` Without Empty Check~~ — FIXED

**Fixed in:** `src/numsim_cas/tensor/simplifier/tensor_simplifier_mul.cpp`

Added `!lhs.data().empty()` guard before `.back()` call.

---

## 4. Unsafe Casts

### ~~4.1 Unchecked `static_cast` in `expression_holder::get<T>()`~~ — FIXED

**Fixed in:** `include/numsim_cas/core/expression_holder.h`

Added `dynamic_cast` assert before `static_cast` in both const and non-const
`get<T>()` overloads. Zero overhead in release, catches type mismatches in debug.

---

## 5. Cross-Domain Consistency

### 5.1 Tensor Domain Missing Pow Simplifier — GAP

**Comparison:**
- Scalar: `scalar/simplifier/scalar_simplifier_pow.h` — full implementation via CRTP + generic dispatch
- Tensor_to_Scalar: `tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_pow.h` — full implementation
- Tensor: **no pow simplifier exists**

The tensor domain has `tensor_pow` and `tensor_power_diff` in `tensor_node_list.h`
(lines 15-16) but no corresponding simplifier. `tensor_operators.h` does not include
any pow simplifier. Instead, `tensor_std.h` has manual `pow()` functions (lines 37-54)
that bypass the tag_invoke CPO pattern used by the other two domains.

**Severity:** GAP — tensor pow expressions are not simplified.

---

### 5.2 Empty/Stub Simplifier Files — GAP

The following files exist but contain no implementation:

| File | Status |
|------|--------|
| `tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_div.h` | Empty — header guard only |
| `tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_mul.h` | Empty — header guard only |
| `tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_add.h` | Empty namespace |
| `tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_sub.h` | Empty — header guard only |

**Severity:** GAP — cross-domain operations (tensor with t2s, t2s with scalar) have
no simplification logic.

---

### 5.3 `tensor_to_scalar_simplifier_div.h` Entirely Commented Out — DISABLED

**File:** `include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_div.h`

The entire file (166 lines) is commented out. Division in the t2s domain is handled
only by converting `a / b` to `a * pow(b, -1)` in `tensor_to_scalar_operators.h`
(lines 112-127). The dedicated div simplifier is disabled.

**Severity:** MEDIUM — division simplification relies entirely on pow conversion.

---

### 5.4 Division Handling Inconsistent Across Domains

| Domain | Dedicated div simplifier | Falls back to pow conversion |
|--------|-------------------------|------------------------------|
| Scalar | `scalar_simplifier_div.h` — active | Yes (`scalar_operators.h:87`) |
| Tensor | `tensor_with_scalar_simplifier_div.h` — active (tensor/scalar only) | Yes (`tensor_operators.h:119`) |
| Tensor_to_Scalar | `tensor_to_scalar_simplifier_div.h` — **commented out** | Yes (`tensor_to_scalar_operators.h:112-127`) |

Scalar has both a dedicated div simplifier and pow fallback. Tensor has scalar-division
simplifier only. T2s has its div simplifier entirely disabled.

---

## 6. Commented-Out Code

### 6.1 Large Blocks of Dead Code in Operator Files

| File | Lines commented | Content |
|------|----------------|---------|
| `tensor_to_scalar/tensor_to_scalar_operators.h` | ~220 lines | Old `operator_overload` structs, pre-tag_invoke |
| `tensor/tensor_operators.h` | ~77 lines | Old `operator_overload` definitions |

These are remnants from the migration to the tag_invoke CPO pattern.

### 6.2 Commented-Out Nodes in Node Lists

- `tensor_to_scalar/tensor_to_scalar_node_list.h:23-25`:
  ```
  //  NEXT(tensor_to_scalar_pow_with_scalar_exponent)
  //  NEXT(tensor_to_scalar_with_scalar_add)
  //  NEXT(tensor_to_scalar_with_scalar_mul)
  ```
- `tensor/tensor_node_list.h:32-35`:
  ```
  // det, adj, skew, vol, dev,
  //  NEXT(tensor_scalar_div)
  // NEXT(tensor_to_scalar_with_tensor_div)
  ```

These represent planned but unimplemented node types.

---

## 7. Naming Issues

### 7.1 "missmatch" Misspelling — 13 Occurrences

The method name `missmatch()` (instead of `mismatch()`) appears consistently in:
- `tensor/data/tensor_data_add.h`
- `tensor/data/tensor_data_to_scalar_wrapper.h` (2x)
- `tensor/data/tensor_data_eval.h` (2x)
- `tensor/data/tensor_data_make_imp.h`
- `tensor/data/tensor_data_sub.h`
- `tensor/data/tensor_data_unary_wrapper.h` (2x)
- `tensor/data/tensor_data_outer_product.h`
- `tensor/data/tensor_data_inner_product.h`
- `tensor/data/tensor_data_basis_change.h`
- `tensor/scalar_tensor_op.h`

Consistently misspelled so it doesn't cause functional issues, but should be renamed.

---

## 8. TODOs

| File | Line | TODO |
|------|------|------|
| `scalar/visitors/scalar_differentiation.h` | 100 | "just copy the vector and manipulate the current entry" |
| `scalar/simplifier/scalar_simplifier_div.h` | 136 | "check if both constants.value() == int --> rational" |
| `scalar/simplifier/scalar_simplifier_pow.h` | 53 | "pow(pow(x,-a), -b) --> pow(x,-a*b) only when x,a,b>0" (conditional simplification) |
| `scalar/scalar_make_constant.h` | 32 | "std::complex support" |

---

## 9. `expression_holder` — Redundant Manual Special Members

**File:** `include/numsim_cas/core/expression_holder.h`

The class manually defines copy/move constructors, copy/move assignment, and destructor
that all do exactly what the compiler-generated versions would do (forward to
`shared_ptr`'s corresponding operations). This is ~40 lines of code that could be
replaced by:

```cpp
expression_holder(expression_holder const&) = default;
expression_holder(expression_holder&&) = default;
expression_holder& operator=(expression_holder const&) = default;
expression_holder& operator=(expression_holder&&) = default;
~expression_holder() = default;
```

Or simply omitted entirely. The compiler-generated special members for a class
containing only a `shared_ptr` are correct and optimal.

**Severity:** LOW — no functional impact, but adds maintenance burden and obscures intent.

---

## 10. `expression_holder` — Reserved Identifiers

**File:** `include/numsim_cas/core/expression_holder.h`

Template parameter names like `_ExprBase` use leading underscore + uppercase, which is
reserved by the C++ standard for the implementation ([lex.name]/3.1). Should be renamed
to `ExprBase` or `ExprBaseT`.

**Severity:** LOW — technically undefined behavior per the standard; unlikely to cause
issues in practice with current compilers.

---

## 11. `expression_holder::operator<` — Non-Deterministic Address Tiebreaker

**File:** `include/numsim_cas/core/expression_holder.h`

When two expressions have a hash collision but are not equal, the comparator falls back
to `std::less<ExprBase const*>` (pointer address). This means the ordering of expressions
in `std::map`/`std::set` can differ between runs, making debugging difficult and
potentially producing different simplified forms across program executions.

**Recommendation:** Introduce a deterministic tiebreaker — either a monotonic creation
counter on `expression` or a deep structural comparison via a `compare_less` visitor.

**Severity:** MEDIUM — affects reproducibility of simplified output. Hash collisions
are rare, but when they occur the results become non-deterministic.

---

## 12. `n_ary_tree::push_back(&&)` — Move Semantics Bug

**File:** `include/numsim_cas/core/n_ary_tree.h`

`push_back(expression_holder<ExprBase>&& __expr)` accepts an rvalue reference but
copies it into the hash map:

```cpp
m_data[__expr.get().hash_value()] = __expr;  // copies, doesn't move
```

Should be:

```cpp
m_data[__expr.get().hash_value()] = std::move(__expr);
```

Since `expression_holder` wraps a `shared_ptr`, the cost is a redundant atomic
increment+decrement per insertion. In hot loops building large sums/products, this
adds measurable overhead.

**Severity:** LOW-MEDIUM — unnecessary atomic operations on every rvalue insertion.

---

## 13. `n_ary_tree::update_hash_value()` — Allocates on Every Call

**File:** `include/numsim_cas/core/n_ary_tree.h`

```cpp
void update_hash_value() {
    std::vector<std::size_t> hashes;
    // ...fills and sorts...
}
```

This allocates a temporary `std::vector` on every hash recomputation. For an
`n_ary_tree` with N children, this is O(N) allocation + O(N log N) sort. Since
hash updates happen during tree construction, this creates unnecessary heap churn.

**Recommendation:** Use a `static thread_local std::vector<size_t>` or
`small_vector<size_t, 8>` to avoid repeated allocations.

**Severity:** LOW — allocation cost is small for typical tree sizes, but adds up
during heavy simplification passes.

---

## 14. `numsim_cas_type_traits.h` — ~350 Lines of Dead Code

**File:** `include/numsim_cas/numsim_cas_type_traits.h`

Of ~630 total lines, approximately 350+ are commented out. The file defines:
- Active: `is_same<T>(expr)`, `is_numeric_expr(expr)`, a few type traits
- Dead: Entire sections of pre-virtual-dispatch type traits (`is_scalar_add`,
  `is_scalar_mul`, etc.), old SFINAE-based helpers, commented-out variant
  type detection

The active portion could be extracted to a ~100-line utility header. The dead code
should be deleted — it references types and patterns that no longer exist in the
codebase.

Additionally, the alias `umap` (line ~25) maps to `std::map` (not `std::unordered_map`),
which is misleading:

```cpp
template <typename Key, typename Value>
using umap = std::map<Key, Value>;
```

**Severity:** LOW (dead code), MEDIUM (misleading `umap` alias — developers reading
the code will assume unordered map semantics).

---

## 15. `scalar_expression` — Unconstrained Forwarding Constructor

**File:** `include/numsim_cas/scalar/scalar_expression.h`

```cpp
template <typename... _Args>
scalar_expression(_Args&&... args)
    : expression_holder_imp<scalar_expression>(std::forward<_Args>(args)...) {}
```

This perfect-forwarding constructor matches any argument list, which means:
- It beats the copy constructor for non-const lvalues
- It can silently convert unrelated types
- It uses reserved identifier names (`_Args`)

**Recommendation:** Add a constraint:

```cpp
template <typename... Args>
requires (sizeof...(Args) != 1 || !std::same_as<std::remove_cvref_t<Args>..., scalar_expression>)
scalar_expression(Args&&... args)
```

Or use a tag type for internal construction.

**Severity:** MEDIUM — can cause surprising overload resolution.

---

## 16. Missing `[[nodiscard]]` on Pure Functions

Throughout the codebase, functions that compute and return values without side effects
lack `[[nodiscard]]`. Key examples:

| Function / Method | File |
|-------------------|------|
| `expression_holder::operator*`, `operator->` | `core/expression_holder.h` |
| `hash_value()`, `id()`, `name()` | `core/expression.h` |
| `n_ary_tree::coeff()`, `size()`, `data()` | `core/n_ary_tree.h` |
| `to_string(expr)` | all domain `*_io.h` files |
| `is_same<T>(expr)`, `is_numeric_expr(expr)` | `numsim_cas_type_traits.h` |
| All simplifier `apply()` methods | `simplifier/*.h` |
| `pow()`, `sin()`, `cos()`, etc. | `scalar_std.h`, `tensor_to_scalar_std.h` |

**Severity:** LOW — no functional impact, but `[[nodiscard]]` helps catch
"forgot to use the result" bugs at compile time.

---

## 17. Build System Issues

### 17.1 Global C++ Standard Override

**File:** `CMakeLists.txt`

```cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

This sets the C++ standard globally for all targets, including third-party
dependencies (GoogleTest, tmech). Should use `target_compile_features` on the
library target only:

```cmake
target_compile_features(numsim_cas_lib PUBLIC cxx_std_23)
```

### 17.2 No Warning Flags on Library Target

The library is compiled without `-Wall -Wextra -Wpedantic` or equivalent.
Warnings are only enabled implicitly through whatever the developer has in their
environment. For a C++23 codebase, recommended minimum:

```cmake
target_compile_options(numsim_cas_lib PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
```

### 17.3 No Sanitizer Support

No CMake option for ASan/UBSan/TSan. For a library with raw pointer casts,
mutable state, and `reinterpret_cast` in tensor data, sanitizer support should
be a build option:

```cmake
option(NUMSIM_CAS_SANITIZERS "Enable ASan+UBSan" OFF)
if(NUMSIM_CAS_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
```

### 17.4 `GLOB_RECURSE` for Sources

**File:** `CMakeLists.txt`

```cmake
file(GLOB_RECURSE NUMSIM_CAS_SOURCES ...)
```

CMake documentation warns against `GLOB_RECURSE` for sources — adding/removing
`.cpp` files won't trigger a re-configure. Developers must remember to re-run
`cmake -B build` after adding files. This has already caused confusion (see the
build failure when adding new example targets without re-running cmake).

**Recommendation:** List source files explicitly, or at minimum add
`CONFIGURE_DEPENDS` to the glob:

```cmake
file(GLOB_RECURSE NUMSIM_CAS_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
```

**Severity:** LOW-MEDIUM collectively — the build system works but doesn't follow
modern CMake best practices.

---

## 18. Test Quality Issues

### 18.1 Empty Test Bodies

Several tests in `TensorToScalarExpressionTest.h` have empty bodies (the test
macro exists but contains no assertions). These give a false sense of coverage.

### 18.2 Tests Encoding Bugs as Expected Behavior

Some tests check for output that includes known simplification failures. For
example, expressions that should simplify to simpler forms are tested against
their unsimplified string representation. When the simplifier is later improved,
these tests will break — they're testing the current bug, not the correct behavior.

**Recommendation:** Mark such tests with a `// TODO: simplification not yet
implemented` comment or use `EXPECT_EQ` with the desired simplified form and
`GTEST_SKIP()` until the simplifier handles the case.

### 18.3 Dead Test File

**File:** `tests/TensorToScalarEvaluatorTest_old.h`

An old copy of the evaluator tests exists alongside the active version. Should
be deleted.

**Severity:** LOW-MEDIUM — test suite gives incomplete picture of correctness.

---

## 19. Public Header Hygiene

### 19.1 `numsim_cas.h` — Over 100 Lines of Commented-Out Includes

**File:** `include/numsim_cas/numsim_cas.h`

The umbrella header has ~160 lines of commented-out includes (lines 93-202) and
~70 lines of commented-out code (lines 205-252). Only ~30 lines are active
includes. The dead code documents the migration history but should be removed —
git history preserves that information.

### 19.2 Inconsistent Include Guards vs `#pragma once`

The codebase uses traditional `#ifndef`/`#define` include guards throughout. This
is fine, but some guard names don't match their file names (e.g., guard says
`SCALAR_OPERATORS_H` for a file in `scalar/` but doesn't include the path prefix).
For a header-heavy library, consider standardizing on `#pragma once` or ensuring
guard names include the full path to prevent collisions.

**Severity:** LOW — cosmetic and maintenance.

---

## Summary

| # | Issue | Severity | Category |
|---|-------|----------|----------|
| ~~1.1~~ | ~~`symbol_base` equality checks hash only~~ | ~~BUG~~ FIXED | Core |
| ~~1.2~~ | ~~`insert_hash` overwrites silently in release~~ | ~~BUG~~ FIXED | Core |
| ~~1.3~~ | ~~`expression_holder::operator<` inconsistent with `==`~~ | ~~DESIGN~~ FIXED | Core |
| ~~1.4~~ | ~~Dead `add_child()` / `init_parameter_pack()` code~~ | ~~DEAD CODE~~ FIXED | Core |
| ~~1.5~~ | ~~Mutable hash not thread-safe~~ | ~~DESIGN~~ FIXED (documented) | Core |
| ~~1.6~~ | ~~Unsafe `static_cast` in `equals_same_type`~~ | ~~SAFETY~~ FIXED | Core |
| ~~2.1~~ | ~~`coeff()` const returns non-const ref~~ | ~~BUG~~ FIXED | Const-correctness |
| ~~3.1~~ | ~~`expression_holder` deref without null check~~ | ~~BUG~~ FIXED | Null safety |
| ~~3.2~~ | ~~`.back()` without empty check~~ | ~~POTENTIAL BUG~~ FIXED | Null safety |
| ~~4.1~~ | ~~Unchecked `static_cast` in `get<T>()`~~ | ~~DESIGN~~ FIXED | Safety |
| 5.1 | No tensor pow simplifier | GAP | Cross-domain |
| 5.2 | Empty stub simplifiers | GAP | Cross-domain |
| 5.3 | t2s div simplifier commented out | MEDIUM | Cross-domain |
| 5.4 | Inconsistent division handling | MEDIUM | Cross-domain |
| 6.1 | ~300 lines of dead operator code | LOW | Cleanup |
| 6.2 | Commented-out nodes in node lists | LOW | Cleanup |
| 7.1 | "missmatch" misspelling (13x) | LOW | Naming |
| 9 | Redundant manual special members | LOW | expression_holder |
| 10 | Reserved identifiers (`_ExprBase`) | LOW | expression_holder |
| 11 | Non-deterministic address tiebreaker | MEDIUM | expression_holder |
| 12 | `push_back(&&)` doesn't move | LOW-MEDIUM | n_ary_tree |
| 13 | `update_hash_value` allocates every call | LOW | n_ary_tree |
| 14 | ~350 lines dead code + misleading `umap` alias | LOW-MEDIUM | type_traits |
| 15 | Unconstrained forwarding constructor | MEDIUM | scalar_expression |
| 16 | Missing `[[nodiscard]]` on pure functions | LOW | All domains |
| 17.1 | Global C++ standard override | LOW | Build |
| 17.2 | No warning flags on library | MEDIUM | Build |
| 17.3 | No sanitizer support | LOW-MEDIUM | Build |
| 17.4 | `GLOB_RECURSE` without `CONFIGURE_DEPENDS` | LOW | Build |
| 18.1 | Empty test bodies | LOW-MEDIUM | Tests |
| 18.2 | Tests encoding bugs as expected | LOW-MEDIUM | Tests |
| 18.3 | Dead test file | LOW | Tests |
| 19.1 | ~160 lines commented-out includes | LOW | Headers |
| 19.2 | Inconsistent include guards | LOW | Headers |

New error types added to `cas_error.h`: `invalid_expression_error`, `internal_error`.
All fixes for issues 1.1-4.1 tested in `tests/CoreBugFixTest.h` (20 tests).

### Priority Recommendations

**High priority (correctness/reliability):**
1. Fix `push_back(&&)` move semantics bug (#12)
2. Add warning flags to library build (#17.2)
3. Constrain forwarding constructor (#15)
4. Fix non-deterministic ordering (#11)

**Medium priority (code quality):**
5. Delete ~350 lines dead code in type_traits (#14)
6. Rename misleading `umap` alias (#14)
7. Add `CONFIGURE_DEPENDS` to GLOB_RECURSE (#17.4)
8. Add sanitizer build option (#17.3)

**Low priority (cleanup/polish):**
9. Delete dead code in operator files and umbrella header (#6.1, #19.1)
10. Fix "missmatch" spelling (#7.1)
11. Add `[[nodiscard]]` incrementally (#16)
12. Remove redundant special members (#9)
13. Delete dead test file (#18.3)

---

## 20. Reviewer's Thoughts

This section is less about individual bugs and more about the library as a whole —
its design philosophy, where it shines, and where I think it will run into friction
as it grows.

### What works well

**The domain abstraction is the right idea.** Factoring three expression domains
(scalar, tensor, tensor-to-scalar) through a shared core with `domain_traits`
specializations is a solid architectural choice. The generic simplifiers in
`core/simplifier/` prove the pattern works: `simplifier_add.h` and `simplifier_mul.h`
are ~500-line algorithms that compile cleanly against both scalar and
tensor-to-scalar domains without code duplication. That's a real win. Most CAS
libraries either give up on generality here or resort to runtime type erasure that
kills performance. This codebase does neither.

**Construction-time simplification is aggressive and correct.** The projector algebra
is a standout. The fact that `dev(dev(X))` simplifies to `dev(X)` and
`vol(A) + dev(A)` simplifies to `sym(A)` at construction time — before any
simplification pass runs — means the AST stays compact from the start. The
`ContractionRule` enum and the algebra table in `projector_algebra.h` are clean and
easy to audit against the mathematical identities they encode. This is the kind of
domain-specific optimization that generic CAS systems can't do.

**The tag_invoke CPO pattern for operators is forward-looking.** Using
customization-point objects (`add_fn`, `sub_fn`, `mul_fn`, `div_fn`) rather than
raw operator overloading means the library can extend to new domains without
touching the operator dispatch layer. The `promote_expr_fn` CPO for cross-domain
promotion is particularly nice — it lets `scalar * tensor` resolve naturally without
a combinatorial explosion of overloads. This is modern C++ done right.

**The test suite is substantial.** 21 test files, 5000+ lines, covering expression
building, simplification, differentiation, evaluation, and substitution across all
three domains. The tensor evaluator tests alone are nearly 35,000 lines. This isn't
a hobby project with a handful of smoke tests — there's real investment in
correctness.

**The examples are well-structured.** Progressive difficulty (scalar basics ->
differentiation -> evaluation -> tensors -> tensor-to-scalar), every example
includes numeric evaluation with expected values, and they compile independently.
They serve as both documentation and implicit integration tests.

### Where I'd push back

**`shared_ptr` everywhere is paying a tax you probably don't need.** Every
`expression_holder` wraps a `shared_ptr<node_type>`. Every copy bumps an atomic
reference count. Every move still has to null-out the source. In a simplification
pass that walks and rebuilds trees of thousands of nodes, the atomic
increment/decrement traffic is measurable. The question is: do you actually need
shared ownership? In most CAS workflows, the expression tree is built, simplified,
evaluated, and discarded. A single-owner model (`unique_ptr` with explicit `clone()`
when you actually need sharing) would eliminate the atomics entirely and make
ownership semantics explicit. If structural sharing is important (e.g., CSE passes
that share subexpressions), you could use a `unique_ptr` with a COW layer, or
even an arena allocator. The current design pays the shared-ownership tax
unconditionally.

**The n_ary_tree hash map is `std::map`, not `std::unordered_map`.** The `umap`
alias in `numsim_cas_type_traits.h` maps to `std::map<Key, Value>` — an ordered
tree, not a hash map despite the name. For the `n_ary_tree`'s child storage, this
means O(log N) insertion and lookup per child. For small N (2-5 children in a
typical add/mul), this is fine. But if expressions grow large (sums with dozens of
terms), a true `std::unordered_map` with the expression hash as key would be O(1)
amortized. More importantly, calling it `umap` when it's an ordered map will
confuse every new contributor who reads the code. Either rename it to `omap` or
switch to `std::unordered_map` — but don't leave a lie in the type alias.

**The visitor pattern creates a rigid coupling between nodes and visitors.** Every
time you add a new node type to a domain's node list macro, every visitor for that
domain must add a new `operator()` overload — even if 90% of visitors don't care
about the new node. This is the classic expression problem. The X-macro
(`NUMSIM_CAS_SCALAR_NODE_LIST`) helps keep things consistent, but it doesn't
eliminate the coupling. Consider whether a default handler (`operator()(expression
const&)`) in visitor base classes could reduce the boilerplate for visitors that
only care about a subset of nodes.

**The codebase is in the middle of a migration and it shows.** There are ~600 lines
of commented-out code in operator files and the umbrella header. The
`numsim_cas_type_traits.h` file is 630 lines, of which ~350 are dead. Entire
simplifier files are disabled (`tensor_to_scalar_simplifier_div.h`). Node types are
commented out in node list macros. An old test file sits alongside its replacement.
The `numsim_cas.h` umbrella header has more commented-out includes than active ones.
This isn't a code quality problem per se — migrations take time — but it creates a
significant cognitive load for anyone trying to understand the current state of the
system. A single cleanup pass deleting everything that's commented out (git
preserves history) would make the codebase dramatically easier to navigate.

**The simplifier architecture is powerful but has scaling concerns.** Each binary
operation creates a visitor, performs a virtual dispatch on the LHS, which calls
a dispatch method that performs pattern matching on the RHS. This is O(1) dispatch
but with a large constant factor (two virtual calls, two visitor constructions,
potential n_ary_tree allocations per operation). For interactive use or small
expressions this is fine. For code generation or large-scale differentiation (where
you might build expressions with thousands of terms), the overhead of constructing
and dispatching a visitor per binary operation could become a bottleneck. Worth
profiling before it becomes a problem.

**Header-source split is inconsistent.** Some functions are defined in headers
(inline), some in `.cpp` files, and the boundary between the two isn't principled.
The `binary_scalar_pow_simplify` linker bug we fixed is a direct consequence of
this inconsistency — a function was declared in a forward header, defined inline
in an implementation header, and the two were included in different translation
units. The rule should be simple: if it's a template or a trivial inline, it goes
in the header. If it has a non-trivial body and isn't a template, it goes in a
`.cpp` file. No third option.

### What I'd do next if this were my project

1. **Delete all commented-out code.** One afternoon, one commit, enormous readability
   improvement. Git has the history if you need it.

2. **Rename `umap` to what it actually is**, or change it to `std::unordered_map`
   with a proper hasher. This is a 5-minute fix with high impact on code clarity.

3. **Add `-Wall -Wextra -Wpedantic` to the library target** (not just tests). The
   tests already have `-Werror` — the library itself should too. You want to know
   about shadowed variables and implicit conversions in your core headers, not just
   your test code.

4. **Profile the simplification hot path.** Build a large expression (e.g.,
   differentiate a sum of 50 terms), time it, and look at where the time goes.
   My guess is `shared_ptr` atomic operations and `std::map` lookups, but profiling
   will tell you for sure.

5. **Finish or delete the stub simplifiers.** Empty header files with just a guard
   are worse than no file at all — they suggest an implementation exists when it
   doesn't. Either implement them or delete them and add a TODO issue.

6. **Consider an arena allocator for expression nodes.** Most CAS workloads allocate
   thousands of small polymorphic objects that all die together. A monotonic arena
   with `shared_ptr`-style aliasing (or just `unique_ptr` into the arena) would
   eliminate per-node heap allocation overhead and improve cache locality.

### Bottom line

This is a well-thought-out library with real domain expertise behind it. The
three-domain architecture, the tag_invoke operator dispatch, the projector algebra,
and the generic simplifier framework are all strong design choices that will pay
dividends as the library grows. The main risks are accidental complexity from the
migration-in-progress state and performance walls from `shared_ptr` + ordered map
in the hot path. Neither is hard to fix, and the foundations are solid.
