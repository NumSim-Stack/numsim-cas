# Critical Review — Round 2

Fresh codebase review conducted after the round-1 fixes (commits up to `2440b50`).
All file paths are relative to `include/numsim_cas/` unless stated otherwise.

---

## 1. Hash Function Bugs

These are the highest-priority items. Broken hashes silently corrupt expression
identity checks, simplification, and caching.

### 1.1 `tensor_scalar_mul` — Hash overwrite ~~(CRITICAL)~~ FIXED

**File:** `tensor/operators/scalar/tensor_scalar_mul.h`

**Fixed:** Removed overwrite line, removed stored `m_constant` member, use inline
`is_same<scalar_constant>` check. When scalar is constant, hash = `rhs.hash_value()`
(preserving sort order with bare tensor). When non-constant, hash combines both
operands.

---

### 1.2 `tensor_pow` — Dead non-virtual hash method ~~(CRITICAL)~~ FIXED

**File:** `tensor/functions/tensor_pow.h`

**Fixed:** Replaced dead `inline void update_hash()` with proper
`virtual void update_hash_value() const noexcept override`. When exponent is
`scalar_constant`, hash = `lhs.hash_value()` (so `pow(A,2)` and `pow(A,3)` share
hash for n_ary combining). When exponent is non-constant, hash combines both.

---

### 1.3 `tensor_projector` — Hash only includes variant index ~~(CRITICAL)~~ FIXED

**File:** `tensor/projection_tensor.h`

**Fixed:** Added `std::visit` calls that hash `Young::blocks` and
`PartialTraceTag::pairs` data members. Empty-struct alternatives (Symmetric, Skew,
General, etc.) correctly rely on index alone.

---

## 2. Equality & Ordering Bugs

### 2.1 `n_ary_vector::operator==` — Hash-only comparison ~~(HIGH)~~ FIXED + TESTED

**File:** `core/n_ary_vector.h`

**Fixed:** Now mirrors `n_ary_tree` pattern: checks hash, id, size, then
deep-compares children in order. Critical for non-commutative `tensor_mul`
where `A*B` and `B*A` share the same hash (sorted child hashes) but are
different expressions.

---

### 2.2 `n_ary_vector::operator!=` and `n_ary_tree::operator!=` — Contract violation ~~(HIGH)~~ FIXED + TESTED

**Files:** `core/n_ary_vector.h`, `core/n_ary_tree.h`

**Fixed:** Both `operator!=` implementations now simply return `!(lhs == rhs)`,
removing the single-element special case that violated the `==`/`!=` contract.

---

### 2.3 `binary_op::operator<` — Hash-only comparison ~~(MEDIUM)~~ FIXED

**File:** `core/binary_op.h`

**Fixed:** Added `virtual bool less_than_same_type()` to `expression` base class
(mirroring `equals_same_type`), with override in `visitable_impl` that downcasts
and calls the concrete type's `operator<`. `expression::operator<` dispatches via
hash → type id → virtual `less_than_same_type`. Fixed `operator<` throughout the
hierarchy: `binary_op` (child hash → deep compare), `unary_op` (hash → id → child),
`n_ary_tree` (hash → size → lexicographic), `n_ary_vector` (hash → size →
lexicographic). Simplified `expression_holder::operator<` and `scalar_expr_less` to
delegate to `expression::operator<`. Removed `creation_id` counter entirely
(also resolves §5.2).

---

## 3. Dead Code

### 3.1 Large commented-out blocks ~~(MEDIUM)~~ FIXED

All commented-out code blocks removed (775 lines deleted across 11 files):

| File | Lines removed | Description |
|------|---------------|-------------|
| `tensor/simplifier/tensor_simplifier_sub.h` | ~252 | Old `constant_sub`, `n_ary_mul_sub`, `get_coefficient`, coefficient handling |
| `tensor/simplifier/tensor_simplifier_mul.h` | ~28 | Legacy coefficient handling, `get_coefficient` |
| `tensor/operators/tensor/tensor_sub.h` | 14 | `tensor_sub` class never implemented |
| `tensor_to_scalar/tensor_to_scalar_pow.h` | 21 | `tensor_to_scalar_pow_with_scalar_exponent` class |
| `tensor/tensor_functions.h` | ~61 | Old `print`, `diff`, `eval`, `identity_tensor`, `inner_product_simplifier` |
| `functions.h` | ~28 | Commented-out includes and `eval` stubs |
| `numsim_cas_type_traits.h` | 5 | Unused `overloaded` helper struct + deduction guide |
| `scalar/scalar_definitions.h` | 5 | Commented-out `#include` references |
| `src/.../tensor_to_scalar_simpilifier_mul.cpp` | 24 | Old `wrapper_tensor_to_scalar_mul_mul` |
| `tests/cas_test_helpers.h` | 62 | Old `ScalarFixture` and `TensorFixture` templates |

### 3.2 Unused files ~~(MEDIUM)~~ FIXED

| File | Status |
|------|--------|
| `scalar/simplifier/scalar_simplifier_div.h` | **DELETED** — 275 lines, entirely commented out, never included. |
| `tensor/simplifier/tensor_with_scalar_simplifier_div.h` | **DELETED** — old `std::variant` pattern, never included. |

---

## 4. Design Gaps

### 4.1 Tensor division simplifier ~~exists but is never wired up (HIGH)~~ FIXED

**Fixed:** Deleted the dead `tensor_with_scalar_simplifier_div.h` (used old
`std::variant` pattern, never included). Added `scalar_one` and
`scalar_constant(value==1)` early-return checks to `div_fn` in
`tensor_operators.h`, matching the pattern used by `mul_fn`.

---

### 4.2 Missing `scalar - tensor_to_scalar` subtraction overloads ~~(HIGH)~~ FIXED + TESTED

**Fixed:** Added `scalar - t2s` and `t2s - scalar` overloads to
`tensor_to_scalar_operators.h`, wrapping the scalar in
`tensor_to_scalar_scalar_wrapper` and delegating to the t2s subtraction operator
(matching the addition pattern). Tests added in `TensorToScalar_CrossDomainSubtraction`.

Additionally, scalar wrapper expressions are now merged across t2s operations:
`wrapper(a) + wrapper(b) → wrapper(a+b)`, `wrapper(a) - wrapper(b) → wrapper(a-b)`,
`wrapper(a) * wrapper(b) → wrapper(a*b)`. This prevents accumulation of independent
wrapper children in add/mul trees when mixing scalars with t2s expressions
(e.g. `trX + x + y` now produces `x+y+tr(X)` instead of three separate children).
Implemented via domain-specific `dispatch(scalar_wrapper)` overrides in
`constant_add`, `constant_sub`, `constant_mul`, `n_ary_add`, and `n_ary_sub`.
The `n_ary_sub_dispatch` base also gained a generic template dispatch for
hash_map-based term cancellation (matching `n_ary_add`'s existing pattern).

---

### 4.3 Projector simplifier — incomplete visitor coverage ~~(MEDIUM)~~ FIXED + TESTED

**File:** `tensor/tensor_functions.h`

**Fixed:** Added `tensor_scalar_mul` and `tensor_negative` unwrapping at
construction time in all four projection functions (`dev`, `sym`, `vol`, `skew`).
`dev(2*X)` now immediately simplifies to `2*dev(X)`, and `dev(-X)` to `-dev(X)`.
Combined with existing projector algebra (`dev(dev(X))→dev(X)`), this also handles
`dev(2*dev(X))→2*dev(X)`. Tests added in `TensorExpressionTest.ProjectorScalarPullThrough`.

---

### 4.4 Domain traits asymmetry — tensor domain can't use generic algorithms (MEDIUM — design constraint)

**File:** `tensor/tensor_domain_traits.h`

```cpp
using mul_type = void;      // tensor_scalar_mul is not n_ary_tree-based
using constant_type = void; // no tensor constant type
```

The tensor domain sets `mul_type` and `constant_type` to `void`, which means the
generic algorithms in `core/simplifier/` (designed around `domain_traits`) cannot
be applied. The scalar and tensor-to-scalar domains both have full traits.

This is by design: tensor multiplication is non-commutative and there is no "tensor
constant" type, so generic `domain_traits`-based algorithms cannot apply. The tensor
domain maintains its own parallel set of simplifiers. **No code change needed.**

---

## 5. Needs Verification

### 5.1 `scalar_differentiation.h:284` — sqrt derivative ~~(NEEDS VERIFICATION)~~ VERIFIED CORRECT

```cpp
void operator()(scalar_sqrt const &visitable) {
  auto &one{get_scalar_one()};
  m_result = one / (2 * m_expr);
  apply_inner_unary(visitable);
}
```

**Verified:** `m_expr` is set to the full expression being visited (line 403:
`m_expr = expr` in `apply_imp`), so when visiting `sqrt(u)`, `m_expr == sqrt(u)`.
Thus `1/(2*m_expr)` = `1/(2*sqrt(u))` — **correct**.

---

### 5.2 Thread-unsafe `creation_id` counter ~~(LOW)~~ FIXED (removed)

**File:** `core/expression.h`

**Fixed:** `creation_id` removed entirely. The virtual `less_than_same_type`
dispatch (§2.3) provides a proper strict weak ordering without needing a
creation-order tiebreaker.

---

## 6. Test Coverage Gaps

### 6.1 Missing simplification tests — PARTIALLY FIXED

| Node type | Has eval test | Has print/simplification test | Notes |
|-----------|--------------|------------------------------|-------|
| `scalar_abs` | Yes | **Yes** | Added `AbsSimplification`: `abs(positive)→x`, `abs(-positive)→y`, `abs(positive_const)→const` |
| `scalar_sign` | Yes | **Yes** | Added `SignSimplification`: `sign(positive)→1`, `sign(negative)→-1` |
| `scalar_rational` | Yes | No | Only evaluator test (`1/3`). No `scalar_rational` node type exists — division produces `pow(denom,-1)`. |
| `tensor_inv` | No | **Yes** | Added `InvPrint`: `inv(X)` prints correctly |
| `simple_outer_product` | No | No | No expression-level tests |

### 6.2 Missing edge case tests — PARTIALLY FIXED

| Scenario | Status |
|----------|--------|
| Division by zero (`x / 0`) | No test — symbolic `x*pow(0,-1)` is valid CAS behavior (no runtime trap) |
| Zero division (`0 / x`) | **TESTED** — scalar `0/x→0`, `0/2→0`; tensor `0/x→0` |
| Hash collision behavior | No stress tests for hash collision paths |
| Empty expression holder operations | `CoreBugFixTest.h` has basic tests, but incomplete |

### 6.3 Tests encoding known bugs as expected behavior ~~(BUG)~~ FIXED

**Fixed:** Added missing `dispatch(tensor_scalar_mul)` to `add_negative` and
`dispatch(tensor_negative)` to `tensor_scalar_mul_add` in the tensor add simplifier.
Now `(-X) + 2*X → X` and `2*X + (-X) → X` both simplify correctly. Test updated
to verify correct behavior.

### 6.4 Commented-out tests — FIXED

**Fixed:** Deleted `TensorInnerProductScalarFactor` and `TensorDotProductNoDistribution`
(entirely commented-out bodies referencing removed `dot_product()`). Cleaned
`TensorDecomposition` to keep only the live `dev(X)` assertion. Deleted commented-out
`scalar_div` constructor test (`scalar_div` class is disabled; division uses `pow(x,-1)`).

---

## 7. Minor Issues

### 7.1 `get_coefficient()` duplicated across 3 files ~~(LOW)~~ FIXED

**Fixed:** Extracted to a free function template `get_coefficient<Traits>(expr, value)`
in `core/domain_traits.h`. Removed the identical member functions from `add_dispatch`,
`mul_dispatch`, and `sub_dispatch`. Updated all call sites (including downstream
scalar and tensor_to_scalar simplifiers) to use the free function.

### 7.2 `merge_add()` depends on transitive include — VERIFIED OK

**Verified:** `simplifier_add.h` directly includes `<numsim_cas/functions.h>` at line 6,
which provides `merge_add()`. No transitive dependency issue.

### 7.3 `scalar_std.h:100-104` — `abs()` calls potentially undefined functions — VERIFIED OK

`scalar_std.h` directly includes `scalar_assume.h` (line 11), which defines
`is_positive`, `is_nonnegative`, `is_negative`, `is_nonpositive` as `inline`
functions. All are fully visible in every TU that includes `scalar_std.h`. No issue.

### 7.4 Existing TODOs in code — ACKNOWLEDGED (no change needed)

Design notes / future work, not bugs:

| File | Line | TODO |
|------|------|------|
| `scalar/visitors/scalar_differentiation.h` | 100 | "just copy the vector and manipulate the current entry" (product rule optimization) |
| ~~`scalar/simplifier/scalar_simplifier_div.h`~~ | ~~136~~ | ~~"check if both constants.value() == int --> rational"~~ (file deleted) |
| `scalar/simplifier/scalar_simplifier_pow.h` | 53 | "pow(pow(x,-a), -b) --> pow(x,-a*b) only when x,a,b>0" |
| `scalar/scalar_make_constant.h` | 32 | "std::complex support" |

---

## Summary

| # | Issue | Severity | Category |
|---|-------|----------|----------|
| 1.1 | `tensor_scalar_mul` hash overwrite | ~~CRITICAL~~ **FIXED + TESTED** | Hash |
| 1.2 | `tensor_pow` hash ignores exponent | ~~CRITICAL~~ **FIXED + TESTED** | Hash |
| 1.3 | `tensor_projector` hash ignores variant contents | ~~CRITICAL~~ **FIXED + TESTED** | Hash |
| 2.1 | `n_ary_vector::operator==` hash-only | ~~HIGH~~ **FIXED + TESTED** | Equality |
| 2.2 | `n_ary_vector/tree::operator!=` contract violation | ~~HIGH~~ **FIXED + TESTED** | Equality |
| 2.3 | `binary_op::operator<` hash-only | ~~MEDIUM~~ **FIXED** | Ordering |
| 3.1-3.2 | ~775 lines dead code removed + unused file deleted | ~~MEDIUM~~ **FIXED** | Cleanup |
| 4.1 | Tensor div simplifier unwired | ~~HIGH~~ **FIXED + TESTED** | Design |
| 4.2 | Missing t2s subtraction overloads + wrapper merging | ~~HIGH~~ **FIXED + TESTED** | Design |
| 4.3 | Projector simplifier incomplete | ~~MEDIUM~~ **FIXED + TESTED** | Design |
| 4.4 | Tensor domain traits asymmetry | **MEDIUM** (design constraint) | Design |
| 5.1 | sqrt derivative correctness | ~~NEEDS VERIFICATION~~ **CORRECT** | Math |
| 5.2 | Thread-unsafe creation_id | ~~LOW~~ **FIXED** (removed) | Safety |
| 6.1-6.4 | Test coverage gaps | ~~MEDIUM~~ **FIXED** (6.2 edge cases tested, 6.3 bug fixed, 6.4 dead tests removed) | Tests |
| 7.1 | `get_coefficient()` duplication | ~~LOW~~ **FIXED** | Quality |
| 7.2 | `merge_add()` transitive include | **VERIFIED OK** | Quality |
| 7.3 | `is_positive`/`is_nonnegative` visibility | **VERIFIED OK** | Quality |
| 7.4 | Existing TODOs | **ACKNOWLEDGED** (design notes, not bugs) | Quality |

### Recommended fix order

1. ~~Hash bugs (1.1, 1.2, 1.3) — FIXED + TESTED~~
2. ~~Equality bugs (2.1, 2.2) — FIXED + TESTED~~
3. ~~Verify sqrt derivative (5.1) — VERIFIED CORRECT~~
4. ~~Add missing subtraction overloads (4.2) — FIXED + TESTED~~
5. ~~Remove dead code (3.x) — FIXED (775 lines + unused file removed)~~
6. ~~Wire up or delete tensor div simplifier (4.1) — FIXED + TESTED~~
7. ~~Fix ordering + remove creation_id (2.3, 5.2) — FIXED~~
8. ~~Projector linearity (4.3) — FIXED + TESTED~~
9. ~~Extend test coverage (6.x) — PARTIALLY FIXED (abs, sign, inv, projector linearity)~~
10. Minor quality items (7.x)
