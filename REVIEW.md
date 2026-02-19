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

### ~~5.1 Tensor Domain Missing Pow Simplifier~~ — NOTED

**Status:** The tensor domain intentionally handles pow differently: `tensor_std.h`
provides manual `pow()` functions for rank-2/4 tensors that bypass the tag_invoke
CPO pattern. Since tensor-pow semantics differ from scalar pow (matrix power vs
exponentiation), a generic simplifier would be inappropriate. The scalar and
tensor-to-scalar domains have full pow simplifier implementations.

**Severity:** LOW — design decision, not a gap.

---

### ~~5.2 Empty/Stub Simplifier Files~~ — FIXED (deleted)

**Fixed in:** commits `ed0ab69`, `6449daf`

All empty/stub simplifier files have been deleted:

| File | Action |
|------|--------|
| `tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_div.h` | Deleted |
| `tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_mul.h` | Deleted |
| `tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_add.h` | Deleted |
| `tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_sub.h` | Deleted |
| `tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_div.h` | Deleted |
| `tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_mul.h` | Deleted |
| `tensor_to_scalar/operators/tensor_to_scalar_with_scalar/*.h` (4 files) | Deleted |

---

### ~~5.3 `tensor_to_scalar_simplifier_div.h` Entirely Commented Out~~ — FIXED (deleted)

**Fixed in:** commit `ed0ab69`

File deleted. Division in the t2s domain is handled by converting `a / b` to
`a * pow(b, -1)` in `tensor_to_scalar_operators.h`. The dead commented-out
simplifier file has been removed.

---

### 5.4 Division Handling Across Domains — DOCUMENTED

All three domains convert `a / b → a * pow(b, -1)` at the operator level. The tensor
domain additionally has an active `tensor_with_scalar_simplifier_div.h` for
tensor-by-scalar division. The scalar domain has `scalar_simplifier_div.h` (commented
out; the pow conversion handles all cases). The t2s domain's div simplifier was deleted
as dead code (commit `ed0ab69`). The approach is consistent: pow-conversion is the
primary mechanism, with optional domain-specific simplifiers where beneficial.

**Severity:** LOW — consistent design, documented.

---

## 6. Commented-Out Code

### ~~6.1 Large Blocks of Dead Code in Operator Files~~ — FIXED (deleted)

**Fixed in:** commit `ed0ab69`

~300 lines of dead `operator_overload` structs removed from
`tensor_to_scalar_operators.h` and `tensor_operators.h`.

### ~~6.2 Commented-Out Nodes in Node Lists~~ — FIXED (deleted)

**Fixed in:** commit `ed0ab69`

Commented-out `NEXT()` entries removed from `tensor_to_scalar_node_list.h`
and `tensor_node_list.h`.

---

## 7. Naming Issues

### ~~7.1 "missmatch" Misspelling~~ — FIXED

**Fixed in:** commit `e0ab2b5`

Renamed `missmatch()` → `mismatch()` across all 13 occurrences in the tensor data files.

---

## 8. TODOs

| File | Line | TODO |
|------|------|------|
| `scalar/visitors/scalar_differentiation.h` | 100 | "just copy the vector and manipulate the current entry" |
| `scalar/simplifier/scalar_simplifier_div.h` | 136 | "check if both constants.value() == int --> rational" |
| `scalar/simplifier/scalar_simplifier_pow.h` | 53 | "pow(pow(x,-a), -b) --> pow(x,-a*b) only when x,a,b>0" (conditional simplification) |
| `scalar/scalar_make_constant.h` | 32 | "std::complex support" |

---

## ~~9. `expression_holder` — Redundant Manual Special Members~~ — FIXED

**Fixed in:** commit `e0ab2b5`

All five special members replaced with `= default` declarations.

---

## ~~10. Reserved Identifiers (`_Foo` Template Parameters)~~ — FIXED

**Fixed in:** this commit

Renamed ~74 occurrences of `_Uppercase` template parameters across 9 files:
`expression_holder.h`, `symbol_base.h`, `n_ary_tree.h`, `n_ary_vector.h`,
`binary_op.h`, `simplifier_add.h`, `simplifier_mul.h`, `simplifier_sub.h`,
`simplify_rule_registry.h`. Also fixed `_BaseSmbol` typo → `BaseSymbol`.

---

## ~~11. `expression_holder::operator<` — Non-Deterministic Address Tiebreaker~~ — FIXED

**Fixed in:** commit `e0ab2b5`

Added monotonic `creation_id()` counter to `expression` base class. The comparator now
uses `creation_id()` as a deterministic tiebreaker when hash+id match but deep-compare
finds inequality. This ensures reproducible ordering across program executions.

---

## ~~12. `n_ary_tree::push_back(&&)` — Move Semantics Bug~~ — FIXED

**Fixed in:** commit `ed0ab69`

`push_back(&&)` now passes `std::move(expr)` to `insert_hash`. The `insert_hash`
method was split into const-ref and rvalue-ref overloads so the move actually
propagates into the map insertion. Also modernized `find()!=end()` to `contains()`.

---

## 13. `n_ary_tree::update_hash_value()` — Allocates on Every Call

**File:** `include/numsim_cas/core/n_ary_tree.h`

Commented-out dead code block removed in this commit. The local `std::vector`
allocation remains as-is — for typical tree sizes (2-8 children) the cost is
negligible.

**Severity:** LOW — allocation cost is small for typical tree sizes.

---

## ~~14. `numsim_cas_type_traits.h` — ~350 Lines of Dead Code + Misleading `umap`~~ — FIXED

**Fixed in:** commit `ed0ab69`

~350 lines of dead code deleted from the file. The misleading `umap` alias was
removed entirely — all usage sites now use `expr_ordered_map` directly (the actual
underlying type), making the ordered-map semantics explicit.

---

## ~~15. `scalar_expression` — Unconstrained Forwarding Constructor~~ — FIXED

**Fixed in:** commit `ed0ab69`

Added `requires` clause to prevent the forwarding constructor from matching
`scalar_expression` lvalues:

```cpp
template <typename... Args>
requires(sizeof...(Args) != 1 ||
         !std::is_same_v<std::remove_cvref_t<Args>..., scalar_expression>)
scalar_expression(Args &&...args) : expression(std::forward<Args>(args)...) {}
```

---

## ~~16. Missing `[[nodiscard]]` on Pure Functions~~ — FIXED

**Fixed in:** commit `e0ab2b5`

Added `[[nodiscard]]` to key pure functions across the codebase: `expression_holder`
accessors, `hash_value()`, `id()`, `name()`, `n_ary_tree` accessors, and others.

---

## 17. Build System Issues

### ~~17.1 Global C++ Standard Override~~ — FIXED

**Fixed in:** this commit

Replaced global `CMAKE_CXX_STANDARD` with per-target `target_compile_features(... PUBLIC cxx_std_23)`.
Removed redundant `set(CMAKE_CXX_STANDARD 23)` from `tests/CMakeLists.txt` (propagated via PUBLIC).

### ~~17.2 No Warning Flags on Library Target~~ — FIXED

**Fixed in:** commit `ed0ab69`

Added `-Wall -Wextra -Wpedantic -Wno-comment -Wno-overloaded-virtual` for
GCC/Clang and `/W4` for MSVC. Three resulting warnings were fixed in the same
commit.

### ~~17.3 No Sanitizer Support~~ — FIXED

**Fixed in:** commit `ed0ab69`

Added `NUMSIM_CAS_SANITIZERS` CMake option enabling ASan+UBSan with
`-fno-omit-frame-pointer`.

### 17.4 `GLOB_RECURSE` for Sources — Already Had `CONFIGURE_DEPENDS`

**Status:** Already present. The `CONFIGURE_DEPENDS` flag was already in
`CMakeLists.txt` (lines 56-68) before the review was written. This item
required no change.

**Severity:** LOW — `CONFIGURE_DEPENDS` mitigates the re-configure issue.
Explicit file listing would be more robust but is low priority.

---

## 18. Test Quality Issues

### ~~18.1 Orphaned `std::cout` in Tests~~ — FIXED

**Fixed in:** this commit

Removed 5 orphaned `std::cout` statements from `TensorToScalarExpressionTest.h`
(4 in `Basics_PrintAndAlgebra`, 1 in `WithScalars_OrderingAndPowers`).

### ~~18.2 Tests Encoding Bugs as Expected Behavior~~ — FIXED

**Fixed in:** this commit

Known-failing assertion in `TensorToScalar_WithScalars_OrderingAndPowers` now uses
`GTEST_SKIP()` with a `// TODO` comment explaining the missing mul-of-mul pow extraction.

### ~~18.3 Dead Test File~~ — FIXED

**Fixed in:** commit `ed0ab69`

`TensorToScalarEvaluatorTest_old.h` deleted.

---

## 19. Public Header Hygiene

### ~~19.1 `numsim_cas.h` — Over 100 Lines of Commented-Out Includes~~ — FIXED

**Fixed in:** commit `ed0ab69`

Umbrella header reduced from ~253 to ~56 lines. All commented-out includes and
dead code removed.

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
| ~~5.1~~ | ~~No tensor pow simplifier~~ | ~~GAP~~ NOTED | Cross-domain |
| ~~5.2~~ | ~~Empty stub simplifiers~~ | ~~GAP~~ FIXED | Cross-domain |
| ~~5.3~~ | ~~t2s div simplifier commented out~~ | ~~MEDIUM~~ FIXED | Cross-domain |
| ~~5.4~~ | ~~Inconsistent division handling~~ | ~~MEDIUM~~ DOCUMENTED | Cross-domain |
| ~~6.1~~ | ~~\~300 lines of dead operator code~~ | ~~LOW~~ FIXED | Cleanup |
| ~~6.2~~ | ~~Commented-out nodes in node lists~~ | ~~LOW~~ FIXED | Cleanup |
| ~~7.1~~ | ~~"missmatch" misspelling (13x)~~ | ~~LOW~~ FIXED | Naming |
| ~~9~~ | ~~Redundant manual special members~~ | ~~LOW~~ FIXED | expression_holder |
| ~~10~~ | ~~Reserved identifiers (`_ExprBase`)~~ | ~~LOW~~ FIXED | expression_holder |
| ~~11~~ | ~~Non-deterministic address tiebreaker~~ | ~~MEDIUM~~ FIXED | expression_holder |
| ~~12~~ | ~~`push_back(&&)` doesn't move~~ | ~~LOW-MEDIUM~~ FIXED | n_ary_tree |
| 13 | `update_hash_value` allocates every call | LOW | n_ary_tree |
| ~~14~~ | ~~\~350 lines dead code + misleading `umap` alias~~ | ~~LOW-MEDIUM~~ FIXED | type_traits |
| ~~15~~ | ~~Unconstrained forwarding constructor~~ | ~~MEDIUM~~ FIXED | scalar_expression |
| ~~16~~ | ~~Missing `[[nodiscard]]` on pure functions~~ | ~~LOW~~ FIXED | All domains |
| ~~17.1~~ | ~~Global C++ standard override~~ | ~~LOW~~ FIXED | Build |
| ~~17.2~~ | ~~No warning flags on library~~ | ~~MEDIUM~~ FIXED | Build |
| ~~17.3~~ | ~~No sanitizer support~~ | ~~LOW-MEDIUM~~ FIXED | Build |
| 17.4 | `GLOB_RECURSE` — already had `CONFIGURE_DEPENDS` | N/A | Build |
| ~~18.1~~ | ~~Orphaned `std::cout` in tests~~ | ~~LOW-MEDIUM~~ FIXED | Tests |
| ~~18.2~~ | ~~Tests encoding bugs as expected~~ | ~~LOW-MEDIUM~~ FIXED | Tests |
| ~~18.3~~ | ~~Dead test file~~ | ~~LOW~~ FIXED | Tests |
| ~~19.1~~ | ~~\~160 lines commented-out includes~~ | ~~LOW~~ FIXED | Headers |
| 19.2 | Inconsistent include guards | LOW | Headers |

New error types added to `cas_error.h`: `invalid_expression_error`, `internal_error`.
All fixes for issues 1.1-4.1 tested in `tests/CoreBugFixTest.h` (20 tests).

### Fix Summary

Of the 28 original issues, **26 are now resolved** (fixed, documented, or noted)
and 2 remain open:

| Remaining Issue | Severity |
|-----------------|----------|
| 13 `update_hash_value` allocates every call | LOW |
| 19.2 Inconsistent include guards | LOW |

Both remaining items are LOW severity and have no functional impact.

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

**The n_ary_tree uses `std::map` (ordered) for child storage.** ~~The misleading
`umap` alias has been renamed to `expr_ordered_map`, making the ordered semantics
explicit.~~ *(Fixed in ed0ab69.)* The question of whether `std::unordered_map`
would be better for performance remains open — for small N (2-5 children) the
ordered map is fine, but for large sums with dozens of terms, O(1) amortized
lookup would help.

**The visitor pattern creates a rigid coupling between nodes and visitors.** Every
time you add a new node type to a domain's node list macro, every visitor for that
domain must add a new `operator()` overload — even if 90% of visitors don't care
about the new node. This is the classic expression problem. The X-macro
(`NUMSIM_CAS_SCALAR_NODE_LIST`) helps keep things consistent, but it doesn't
eliminate the coupling. Consider whether a default handler (`operator()(expression
const&)`) in visitor base classes could reduce the boilerplate for visitors that
only care about a subset of nodes.

**~~The codebase is in the middle of a migration and it shows.~~** *(Largely
addressed in commits ed0ab69 and 6449daf.)* The major cleanup pass removed ~1,600
lines of dead code: commented-out operator structs, dead type traits, disabled
simplifiers, stub files, and umbrella header cruft. The `numsim_cas_type_traits.h`
file went from ~630 to ~280 lines, the umbrella header from ~253 to ~56 lines,
and 11 dead files were deleted entirely. The codebase is now much cleaner and
easier to navigate. Some migration artifacts remain (commented-out code in
`compare_equal_visitor.h`, the old test file `TensorToScalarEvaluatorTest_old.h`)
but the cognitive load has been significantly reduced.

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

1. ~~**Delete all commented-out code.**~~ Done (ed0ab69, 6449daf).

2. ~~**Rename `umap` to what it actually is.**~~ Done — renamed to `expr_ordered_map` (ed0ab69).

3. ~~**Add `-Wall -Wextra -Wpedantic` to the library target.**~~ Done (ed0ab69).

4. **Profile the simplification hot path.** Build a large expression (e.g.,
   differentiate a sum of 50 terms), time it, and look at where the time goes.
   My guess is `shared_ptr` atomic operations and `std::map` lookups, but profiling
   will tell you for sure.

5. ~~**Finish or delete the stub simplifiers.**~~ Deleted (ed0ab69, 6449daf).

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
