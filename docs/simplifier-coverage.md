# Simplifier Coverage

> Audit of the simplifier layer across the three expression domains, captured
> for issue [#95](https://github.com/NumSim-Stack/numsim-cas/issues/95). The
> goal is to surface drift, missing rules, and architectural asymmetries so
> they can be tracked rather than rediscovered.
>
> **Scope**: visitor-driven simplification in
> `include/numsim_cas/{core,scalar,tensor,tensor_to_scalar}/simplifier/` only.
> Construction-time simplifications (e.g. `inv(inv(A)) → A` in `tensor_functions.h`,
> `sin(asin(x)) → x` in `scalar_std.h`, `trace(0) → 0` in
> `tensor_to_scalar_functions.cpp`, the `is_trans_of` annotations in
> `tensor_operators.h`) are not in this matrix — they deserve a parallel audit
> as a follow-up. Differentiation, evaluation, printing, and substitution
> visitors are also out of scope.

## Layering

```
core/simplifier/      ← generic dispatchers (templated on Traits)
scalar/simplifier/    ← scalar wrappers (inherit core)
tensor/simplifier/    ← tensor wrappers (hand-written; tensor mul_type is void)
tensor_to_scalar/simplifier/  ← t2s wrappers (inherit core + unwrap pattern)
```

The generic dispatchers parameterise over `domain_traits<Domain>`
([`include/numsim_cas/core/domain_traits.h`](../include/numsim_cas/core/domain_traits.h)),
which exposes per-domain type aliases (`add_type`, `mul_type`, `negative_type`,
…) and numeric hooks (`try_numeric`, `zero`, `one`, `make_constant`). Specialisations
live in `scalar/scalar_domain_traits.h` and `tensor/tensor_domain_traits.h`.

## Generic dispatcher inventory

### Add — 7 dispatchers ([`simplifier_add.h`](../include/numsim_cas/core/simplifier/simplifier_add.h))

| Dispatcher | LHS | Key overloads (rhs type) | Source |
|---|---|---|---|
| `add_dispatch` | base | `Expr` (fallback), `zero_type`, `negative_type`, `add_type` | `:19` |
| `constant_add_dispatch` | constant | `constant_type`, `add_type`, `one_type`, `negative_type` | `:111` |
| `one_add_dispatch` | one | `constant_type`, `add_type`, `one_type`, `negative_type` | `:185` |
| `n_ary_add_dispatch` | add (n-ary sum) | `constant_type`, `one_type`, `symbol_type`, `add_type`, `negative_type` | `:245` |
| `n_ary_mul_add_dispatch` | mul | `symbol_type`, `mul_type` | `:349` |
| `symbol_add_dispatch` | symbol | `symbol_type`, `mul_type` | `:403` |
| `negative_add_dispatch` | negative | `negative_type`, `add_type`, `constant_type` | `:452` |

### Sub — 7 dispatchers ([`simplifier_sub.h`](../include/numsim_cas/core/simplifier/simplifier_sub.h))

| Dispatcher | LHS | Key overloads (rhs type) | Source |
|---|---|---|---|
| `sub_dispatch` | base | `Expr` (fallback), `zero_type` | `:18` |
| `negative_sub_dispatch` | negative | `add_type` | `:83` |
| `constant_sub_dispatch` | constant | `constant_type`, `add_type`, `one_type` | `:114` |
| `n_ary_sub_dispatch` | add | `constant_type`, `one_type`, `symbol_type`, `add_type` | `:169` |
| `n_ary_mul_sub_dispatch` | mul | `symbol_type`, `mul_type` | `:268` |
| `symbol_sub_dispatch` | symbol | `symbol_type`, `mul_type` | `:345` |
| `one_sub_dispatch` | one | `constant_type` | `:396` |

Naming is parallel to the add side, but **overload set is narrower** — e.g.
`negative_sub_dispatch` only handles `add_type` rhs (vs add side's four).

### Mul — 1 dispatcher ([`simplifier_mul.h`](../include/numsim_cas/core/simplifier/simplifier_mul.h))

| Dispatcher | LHS | Key overloads (rhs type) | Source |
|---|---|---|---|
| `mul_dispatch` | base | `Expr` (fallback), `zero_type`, `one_type`, `mul_type` | `:16` |

**No LHS-specialised siblings.** Every rule for multiplication of compound
expressions (e.g. mul × mul, symbol × mul) goes through the base dispatcher's
`get_default()` path or is handled per-domain. See *Asymmetry findings* below.

### Pow — 3 dispatchers ([`simplifier_pow.h`](../include/numsim_cas/core/simplifier/simplifier_pow.h))

| Dispatcher | LHS | Key overloads (rhs type) | Source |
|---|---|---|---|
| `pow_dispatch` | base | `Expr` (fallback) | `:15` |
| `pow_pow_dispatch` | pow | `Expr`, `negative_type` | `:67` |
| `mul_pow_dispatch` | mul | `negative_type`, `Expr` | `:97` |

## Per-domain wrappers

### Scalar — full coverage, 1 domain-specific rule

| Family | Wrapper classes | Notes |
|---|---|---|
| Add | `add_default_visitor`, `constant_add`, `add_scalar_one`, `n_ary_add`, `n_ary_mul_add`, `symbol_add`, `add_negative`, **`pow_add`** | All inherit matching `*_dispatch<scalar_traits>`. `pow_add` adds `sin²(x) + cos²(x) → 1` ([`scalar_simplifier_add.h:127`](../include/numsim_cas/scalar/simplifier/scalar_simplifier_add.h)) — domain-specific. |
| Sub | `sub_default_visitor`, `negative_sub`, `constant_sub`, `n_ary_sub`, `n_ary_mul_sub`, `symbol_sub`, `scalar_one_sub` | All inherit matching `*_dispatch<scalar_traits>`. No added overloads. |
| Mul | `mul_default<Derived>`, `constant_mul`, `n_ary_mul`, `exp_mul`, `scalar_pow_mul`, `symbol_mul` | `exp_mul`: `exp(a)·exp(b) → exp(a+b)`. `scalar_pow_mul`: `pow(x,a)·pow(x,b) → pow(x,a+b)`. |
| Pow | `pow_default<Derived>`, `pow_pow`, `mul_pow` | Inherits generic; subclasses specialise. |

### Tensor — hand-written; no pow simplifier

| Family | Wrapper classes | Notes |
|---|---|---|
| Add | `add_default<Derived>`, `n_ary_add`, `symbol_add`, **`tensor_scalar_mul_add`**, `add_negative` | **Does not inherit generic.** Hand-written because `tensor_traits::mul_type` is `void` (no n-ary scalar multiplication node in this domain — see [`tensor_domain_traits.h`](../include/numsim_cas/tensor/tensor_domain_traits.h)). `tensor_scalar_mul_add` is domain-specific. |
| Sub | `sub_default<Derived>`, `negative_sub`, `n_ary_sub`, `symbol_sub` | Same architectural note as add. |
| Mul | `mul_default<Derived>`, `tensor_pow_mul`, `kronecker_delta_mul`, `symbol_mul`, `n_ary_mul` | Hand-written. `kronecker_delta_mul` handles index contraction; `symbol_mul`: `x·x → pow(x,2)`. |
| Pow | **— absent —** | No `tensor_simplifier_pow.h` file. Rules like `pow(pow(A,m), n) → pow(A, m·n)` and `pow(inv(A), n)` have no construction-time handling. → tracked separately. |

Additional domain-specific simplifiers (not in the dispatcher hierarchy):

- [`tensor_inner_product_simplifier.h`](../include/numsim_cas/tensor/simplifier/tensor_inner_product_simplifier.h) — `inner_product` reductions for Kronecker delta, identity tensor, outer product, scalar mul.
- [`tensor_projector_simplifier.h`](../include/numsim_cas/tensor/simplifier/tensor_projector_simplifier.h) — projector algebra: `P:P → P`, `P_i:P_j → 0`, `P_vol:A + P_dev:A → P_sym:A`, etc.
- [`tensor_with_scalar_simplifier_mul.h`](../include/numsim_cas/tensor/simplifier/tensor_with_scalar_simplifier_mul.h) — `mul_base` for scalar × tensor coefficient merging.

### Tensor-to-scalar — generic coverage + unwrap/rewrap pattern

| Family | Wrapper classes | Notes |
|---|---|---|
| Add | `add_default_visitor`, `n_ary_add`, `negative_add`, `constant_add`, `one_add`, `n_ary_mul_add` | All inherit generic `*_dispatch<tensor_to_scalar_traits>`. **Domain-specific**: `n_ary_add::dispatch(tensor_to_scalar_scalar_wrapper)` unwraps the wrapper, runs the scalar-domain op, rewraps. Same pattern in `constant_add`. |
| Sub | `sub_default_visitor`, `negative_sub`, `n_ary_sub`, `constant_sub`, `one_sub`, `n_ary_mul_sub` | Same as add. The unwrap/rewrap dispatch lives in both `n_ary_sub` and `constant_sub`. |
| Mul | `mul_default<Derived>`, `constant_mul` | `constant_mul` adds wrapper-aware dispatches. |
| Pow | `pow_default_visitor`, `pow_pow_visitor`, `mul_pow_visitor` | Virtual overrides in `.cpp` for `pow()` ADL resolution. |

## Asymmetry findings

Ranked by impact. Each gap that is not a deliberate architectural choice gets
a tracking issue.

### Material gaps (filed)

1. **No tensor pow simplifier.** Entire `tensor/simplifier/tensor_simplifier_pow.h`
   is missing. Tensor `pow` expressions are constructed via
   [`tensor/functions/tensor_pow.h`](../include/numsim_cas/tensor/functions/tensor_pow.h)
   and the `pow()` free function but receive no simplifier-driven rewrites
   (`pow(pow(A, m), n) → pow(A, m·n)`, `pow(inv(A), n) → inv(pow(A, n))`, etc.).
   Tracked as [#96](https://github.com/NumSim-Stack/numsim-cas/issues/96).

2. **No `n_ary_mul_mul_dispatch`.** The mul family has only the base
   `mul_dispatch` ([`simplifier_mul.h:16`](../include/numsim_cas/core/simplifier/simplifier_mul.h)).
   Rules like `(c₁·x·y) · (c₂·x·z) → c₁·c₂·pow(x,2)·y·z` aren't reachable
   without a dedicated LHS=mul dispatcher. Compare with the add side's
   `n_ary_add_dispatch`, which exists.
   Tracked as [#97](https://github.com/NumSim-Stack/numsim-cas/issues/97).

3. **`pow_add` Pythagorean identity is scalar-only.** `sin²(x) + cos²(x) → 1`
   is implemented in [`scalar_simplifier_add.h:127`](../include/numsim_cas/scalar/simplifier/scalar_simplifier_add.h)
   and [`src/numsim_cas/scalar/simplifier/scalar_simplifier_add.cpp`](../src/numsim_cas/scalar/simplifier/scalar_simplifier_add.cpp).
   **Deliberate** — tensor and t2s domains have no trig functions, so the rule
   has no analog.

4. **T2s `unwrap/rewrap` pattern is t2s-only.** `n_ary_add::dispatch(tensor_to_scalar_scalar_wrapper)`
   unwraps a scalar-result wrapper, runs the scalar-domain op, and rewraps.
   **Deliberate** — scalar/tensor domains don't have an analogous cross-domain
   wrapper to unwrap.

### Watch list — duplication candidates (not extracted)

Five pairs of code blocks across add/sub mirrors with similar shape. None are
big enough to justify an extraction today (drift-risk vs. API-clarity trade
doesn't favor extracting 3–5 line blocks). Listed here so a future pass can
revisit if any grow.

| Pair | LHS+op file:line | RHS-side file:line | Shape |
|---|---|---|---|
| Constant ± constant numeric fold | `simplifier_add.h:125` | `simplifier_sub.h:126` | `try_numeric` on both, combine, return zero or constant |
| Coefficient handling for `constant ± add` | `simplifier_add.h:138` | `simplifier_sub.h:139` | Extract coeff via `get_coefficient<Traits>(rhs, 0)`, combine, rebuild add |
| `1 ± add` rebuild | `simplifier_add.h:210` | `simplifier_sub.h:152` | Same coeff-extract / rebuild pattern as above |
| `n_ary_mul ± mul` hash-equality merge | `simplifier_add.h:378` | `simplifier_sub.h:306` | Hash check; `get_coefficient<Traits>(lhs, 1)` + rhs coefficient; combine |
| `symbol_map` find / combine / `merge_or_insert` (`x+y+z ± x`) | `simplifier_add.h:291` | `simplifier_sub.h:243` | Same shape, differs only by `+` / `-` in the combine step |
| Merge two adds (`(c_l + a + b) ± (c_r + a + d)`) | `simplifier_add.h:306` | `simplifier_sub.h:217` | **Legitimately divergent post-PR #98.** Sub has zero-filtering on combined children, validity-guarded coeff, and trivial-result collapse — none of which the add side needs because `+` rarely cancels children to zero. Do not extract a shared helper. |

### Known semantic defect (in progress on this branch)

- **`n_ary_sub_dispatch::dispatch(add_type)` uses `+` where `-` is correct.**
  `simplifier_sub.h:216–238` was authored as a copy of the add-side merge
  routine and never properly converted to subtraction semantics. The coeff
  combination is unguarded against invalid coefficients, and child-child
  combination is `+` instead of `-`. Tracked as
  [#91](https://github.com/NumSim-Stack/numsim-cas/issues/91). Fixed in commit
  3 of this branch.

## Cross-references

- PR [#90](https://github.com/NumSim-Stack/numsim-cas/pull/90) — context for the audit.
- Issue [#91](https://github.com/NumSim-Stack/numsim-cas/issues/91) — sub-side semantic defect (fixed in this branch).
- Issue [#92](https://github.com/NumSim-Stack/numsim-cas/issues/92) — deterministic regression for `merge_or_insert` loop.
- Issue [#17](https://github.com/NumSim-Stack/numsim-cas/issues/17) — mul-side merge tracker.
- Issue [#71](https://github.com/NumSim-Stack/numsim-cas/issues/71) — tensor inv simplifier rules (scalar-factor extraction half).
- Issue [#75](https://github.com/NumSim-Stack/numsim-cas/issues/75) — specific scalar-tensor mul case.
- Issue [#96](https://github.com/NumSim-Stack/numsim-cas/issues/96) — missing tensor pow simplifier (filed from this audit).
- Issue [#97](https://github.com/NumSim-Stack/numsim-cas/issues/97) — missing `n_ary_mul_mul_dispatch` (filed from this audit).
- Issue [#99](https://github.com/NumSim-Stack/numsim-cas/issues/99) — centralise the trivial-result collapse pattern (filed from PR #98 review).
