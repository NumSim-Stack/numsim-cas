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
If equal → returns false. On hash collision → uses `std::less<ExprBase const*>`
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

New error types added to `cas_error.h`: `invalid_expression_error`, `internal_error`.
All fixes tested in `tests/CoreBugFixTest.h` (20 tests).
Issues 1.1–4.1 all resolved.
