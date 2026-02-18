# Symbolic Differentiation

This module implements **automatic symbolic differentiation** of tensor and
tensor-to-scalar expressions with respect to tensor variables. Together with the
pre-existing scalar differentiation, the library now covers all three expression
domains.

| Domain combination | Signature | Result type |
|-|-|-|
| scalar / scalar | $\dfrac{\partial f}{\partial x}$ | `scalar_expression` |
| tensor / tensor | $\dfrac{\partial \mathbf{A}}{\partial \mathbf{X}}$ | `tensor_expression` |
| tensor-to-scalar / tensor | $\dfrac{\partial \varphi}{\partial \mathbf{X}}$ | `tensor_expression` |

```{contents}
:depth: 3
```

---

## Mathematical Foundations

### Index notation conventions

Throughout this document indices are written in subscript form.
The library stores indices **0-based** internally; user-facing constructors
that accept `sequence{1, 2}` initialiser-lists convert from 1-based to 0-based
automatically.

| Symbol | Meaning |
|--------|---------|
| $\delta_{ij}$ | Kronecker delta (`kronecker_delta`, rank 2) |
| $\mathbb{I}_{ijkl}$ | Fourth-order identity tensor (`identity_tensor`, rank 4) |
| $\mathbf{0}$ | Zero tensor (`tensor_zero`, any rank) |

### Return type and rank rules

For a tensor expression $\mathbf{A}$ of rank $r_A$ differentiated with respect
to a tensor variable $\mathbf{X}$ of rank $r_X$:

$$
\operatorname{rank}\!\left(\frac{\partial \mathbf{A}}{\partial \mathbf{X}}\right)
= r_A + r_X
$$

For a scalar-valued tensor function $\varphi(\mathbf{X})$:

$$
\operatorname{rank}\!\left(\frac{\partial \varphi}{\partial \mathbf{X}}\right)
= r_X
$$

---

## Architecture

### Customisation Point Object (CPO)

Differentiation is invoked through a single entry point defined in
`core/diff.h`:

```cpp
namespace numsim::cas {
  // Ergonomic call — deduces domain types automatically
  auto result = diff(expr, arg);
}
```

Under the hood `diff` is a `tag_invoke`-based CPO.  Each domain provides a
`tag_invoke` overload that constructs the appropriate visitor:

```cpp
// tensor / tensor  (tensor_diff.h)
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn,
           std::type_identity<tensor_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &expr,
           expression_holder<tensor_expression> const &arg);

// tensor_to_scalar / tensor  (tensor_to_scalar_diff.h)
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn,
           std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_to_scalar_expression> const &expr,
           expression_holder<tensor_expression> const &arg);
```

### Virtual visitor pattern

Each domain's differentiation logic lives in a visitor class that inherits
the domain's const-visitor base:

| Visitor class | Base class | Result member |
|---|---|---|
| `scalar_differentiation` | `scalar_visitor_const_t` | `scalar_expression` |
| `tensor_differentiation` | `tensor_visitor_const_t` | `tensor_expression` |
| `tensor_to_scalar_differentiation` | `tensor_to_scalar_visitor_const_t` | `tensor_expression` |

The visitor implements `operator()` overloads for every node type in the
domain's node list macro.  Dispatch happens via `accept(*this)`:

```cpp
tensor_holder_t apply(tensor_holder_t const &expr) {
    m_result = tensor_holder_t{};           // reset
    expr.get<tensor_visitable_t>().accept(*this);
    if (!m_result.is_valid())
        return make_expression<tensor_zero>(m_dim, m_rank_result);
    return m_result;
}
```

### Cross-domain recursion

Tensor differentiation can recurse into tensor-to-scalar differentiation
(e.g.\ when differentiating a `tensor_to_scalar_with_tensor_mul` node), and
vice-versa (e.g.\ when the trace handler calls `diff(tensor_child, X)`).

**Circular include resolution.** The `.h` headers *forward-declare* the
cross-domain `tag_invoke` overloads instead of including each other's
`_diff.h` files.  The actual definitions live in `.cpp` translation units that
include both `tensor_diff.h` and `tensor_to_scalar_diff.h`:

```
tensor_differentiation.h          tensor_to_scalar_differentiation.h
  (forward-declares t2s diff)        (forward-declares tensor diff)
         │                                    │
         ▼                                    ▼
tensor_differentiation.cpp        tensor_to_scalar_differentiation.cpp
  #include tensor_diff.h             #include tensor_diff.h
  #include t2s_diff.h                #include t2s_diff.h
```

---

## Tensor Differentiation Rules

The `tensor_differentiation` visitor handles all **20 node types** in the
tensor node list.  Simple rules are defined inline in the header; complex or
cross-domain rules are defined in the `.cpp` file.

### Constants and symbols

| Node | Rule | Location |
|------|------|----------|
| `tensor` (symbol) | $\dfrac{\partial \mathbf{X}}{\partial \mathbf{X}} = \mathbb{I}$, &ensp; $\dfrac{\partial \mathbf{Y}}{\partial \mathbf{X}} = \mathbf{0}$ | `.h` |
| `tensor_zero` | $\mathbf{0}$ | `.h` |
| `kronecker_delta` | $\mathbf{0}$ (constant) | `.h` |
| `identity_tensor` | $\mathbf{0}$ (constant) | `.h` |
| `tensor_projector` | $\mathbf{0}$ (constant) | `.h` |

Variable matching is performed by comparing hash values:

```cpp
void operator()(tensor const &visitable) override {
    if (visitable.hash_value() == m_arg.get().hash_value())
        m_result = make_expression<identity_tensor>(m_dim, m_rank_result);
}
```

### Linearity rules

$$
\frac{\partial}{\partial \mathbf{X}}\!\sum_i \mathbf{A}_i
  = \sum_i \frac{\partial \mathbf{A}_i}{\partial \mathbf{X}}
$$

$$
\frac{\partial (-\mathbf{A})}{\partial \mathbf{X}}
  = -\frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

$$
\frac{\partial (c\,\mathbf{A})}{\partial \mathbf{X}}
  = c\,\frac{\partial \mathbf{A}}{\partial \mathbf{X}}
  \qquad (c \text{ is a scalar expression})
$$

| Node | Location |
|------|----------|
| `tensor_add` | `.h` (iterates `hash_map()`) |
| `tensor_negative` | `.h` |
| `tensor_scalar_mul` | `.h` |

### Deviatoric, volumetric, and symmetry projections

$$
\frac{\partial}{\partial \mathbf{X}} \operatorname{dev}(\mathbf{A})
  = \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
    - \frac{1}{d}\;(\boldsymbol{\delta} \otimes \boldsymbol{\delta})
      : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

$$
\frac{\partial}{\partial \mathbf{X}} \operatorname{vol}(\mathbf{A})
  = \frac{1}{d}\;(\boldsymbol{\delta} \otimes \boldsymbol{\delta})
    : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

$$
\frac{\partial}{\partial \mathbf{X}} \operatorname{sym}(\mathbf{A})
  = \tfrac{1}{2}\!\left(
      \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
      + \operatorname{permute}\!\left(
          \frac{\partial \mathbf{A}}{\partial \mathbf{X}},\;
          \{1,4,3,2\}
        \right)
    \right)
$$

where $d$ is the spatial dimension.

| Node | Location |
|------|----------|
| `tensor_deviatoric` | `.h` |
| `tensor_volumetric` | `.h` |
| `tensor_symmetry` | `.h` |

### Power rule

The `tensor_pow` node $\mathbf{A}^n$ is **not** differentiated directly.
Instead it creates a `tensor_power_diff` node that defers the expansion:

$$
\frac{\partial \mathbf{A}^n}{\partial \mathbf{X}}
  = \sum_{r=0}^{n-1}
    \bigl(\mathbf{A}^r \,\overline{\otimes}\, \mathbf{A}^{n-1-r}\bigr)
    : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

In the current implementation, the `tensor_power_diff` handler contracts the
entire symbolic node with $\frac{\partial \mathbf{A}}{\partial \mathbf{X}}$
via `inner_product`:

```cpp
m_result = inner_product(m_expr, sequence{3, 4}, dA, sequence{1, 2});
```

| Node | Location |
|------|----------|
| `tensor_pow` | `.h` (creates `tensor_power_diff` node) |
| `tensor_power_diff` | `.cpp` |

### Product rules (tensor multiplication)

**Contracted product** (`tensor_mul`, stored in an `n_ary_vector`):

$$
\frac{\partial}{\partial \mathbf{X}}
  (\mathbf{A}_1 \cdot \mathbf{A}_2 \cdots \mathbf{A}_n)
  = \sum_{j=1}^{n}
    \mathbf{A}_1 \cdots
    \frac{\partial \mathbf{A}_j}{\partial \mathbf{X}}
    \cdots \mathbf{A}_n
$$

Each term is assembled by contracting the derivative with the remaining
factors using `inner_product` on adjacent index pairs.

**Outer product** (`simple_outer_product`, stored in an `n_ary_vector`):

Same summation structure but uses `otimes` (no contraction) to rebuild terms.

| Node | Location |
|------|----------|
| `tensor_mul` | `.cpp` |
| `simple_outer_product` | `.cpp` |

### Inverse

For a rank-2 tensor:

$$
\frac{\partial \mathbf{A}^{-1}}{\partial \mathbf{X}}
  = -\mathbf{A}^{-1}\,
    \frac{\partial \mathbf{A}}{\partial \mathbf{X}}\,
    \mathbf{A}^{-1}
$$

implemented as two successive `inner_product` contractions:

```cpp
auto temp = inner_product(invA, sequence{2}, dA, sequence{1});
m_result  = -inner_product(std::move(temp), sequence{3}, invA, sequence{1});
```

| Node | Location |
|------|----------|
| `tensor_inv` | `.cpp` (rank 2 only; higher ranks throw `not_implemented_error`) |

### Inner and outer product wrappers

Both use the product rule with appropriate index shifting.

**Inner product** $\langle \mathbf{A},\, \mathbf{B} \rangle_{I_A, I_B}$:

$$
\frac{\partial}{\partial \mathbf{X}}
  \operatorname{inner}(\mathbf{A}, I_A, \mathbf{B}, I_B)
  = \operatorname{inner}\!\left(
      \frac{\partial \mathbf{A}}{\partial \mathbf{X}}, I_A,
      \mathbf{B}, I_B
    \right)
  + \operatorname{inner}\!\left(
      \mathbf{A}, I_A,
      \frac{\partial \mathbf{B}}{\partial \mathbf{X}}, I_B
    \right)
$$

When the derivative of the left operand introduces extra indices, the result
is reordered with `permute_indices` so that the differentiation indices
appear last.

**Outer product** $\mathbf{A} \otimes \mathbf{B}$: analogous, with outer-product
index bookkeeping.

| Node | Location |
|------|----------|
| `inner_product_wrapper` | `.cpp` |
| `outer_product_wrapper` | `.cpp` |

### Basis change (index permutation)

$$
\frac{\partial}{\partial \mathbf{X}}
  \operatorname{permute}(\mathbf{A}, \pi)
  = \operatorname{permute}\!\left(
      \frac{\partial \mathbf{A}}{\partial \mathbf{X}},\;
      \pi \,\|\, \operatorname{id}_{r_X}
    \right)
$$

The permutation is extended by appending an identity mapping for the extra
indices introduced by the derivative.

| Node | Location |
|------|----------|
| `basis_change_imp` | `.cpp` |

### Cross-domain: scalar-tensor product

For $f \cdot \mathbf{A}$ where $f$ is a `tensor_to_scalar_expression`:

$$
\frac{\partial (f\,\mathbf{A})}{\partial \mathbf{X}}
  = f \cdot \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
  + \mathbf{A} \otimes \frac{\partial f}{\partial \mathbf{X}}
$$

The first term uses `make_expression<tensor_to_scalar_with_tensor_mul>` (since
the `t2s * tensor` operator is not currently enabled).  The second term uses
`otimes`.

| Node | Location |
|------|----------|
| `tensor_to_scalar_with_tensor_mul` | `.cpp` |

---

## Tensor-to-Scalar Differentiation Rules

The `tensor_to_scalar_differentiation` visitor handles all **13 node types** in
the tensor-to-scalar node list.  All methods are defined in the `.cpp` file
since they require cross-domain includes.

### Constants

| Node | Rule |
|------|------|
| `tensor_to_scalar_zero` | $\mathbf{0}$ |
| `tensor_to_scalar_one` | $\mathbf{0}$ |
| `tensor_to_scalar_scalar_wrapper` | $\mathbf{0}$ (no tensor dependency) |

### Linearity and product rules

**Negation:**

$$
\frac{\partial (-\varphi)}{\partial \mathbf{X}}
  = -\frac{\partial \varphi}{\partial \mathbf{X}}
$$

**Addition** (over `n_ary_tree` children; the constant coefficient has zero
derivative):

$$
\frac{\partial}{\partial \mathbf{X}} \sum_i \varphi_i
  = \sum_i \frac{\partial \varphi_i}{\partial \mathbf{X}}
$$

**Multiplication** (over `n_ary_tree` children with coefficient $c$):

$$
\frac{\partial}{\partial \mathbf{X}}
  \bigl(c \cdot \textstyle\prod_i \varphi_i\bigr)
  = c \cdot \sum_j
    \left(
      \frac{\partial \varphi_j}{\partial \mathbf{X}}
      \cdot \prod_{i \ne j} \varphi_i
    \right)
$$

Each term $\frac{\partial \varphi_j}{\partial \mathbf{X}}$ is a **tensor**
(rank $r_X$).  Multiplying by the remaining scalar-valued factors uses
`make_expression<tensor_to_scalar_with_tensor_mul>`.

### Power rule

**Constant exponent** ($h$ independent of $\mathbf{X}$):

$$
\frac{\partial\, g^h}{\partial \mathbf{X}}
  = h\, g^{h-1}\,
    \frac{\partial g}{\partial \mathbf{X}}
$$

**General case** (both $g$ and $h$ depend on $\mathbf{X}$):

$$
\frac{\partial\, g^h}{\partial \mathbf{X}}
  = g^{h-1} \left(
      h \cdot \frac{\partial g}{\partial \mathbf{X}}
      + \frac{\partial h}{\partial \mathbf{X}} \cdot \ln(g) \cdot g
    \right)
$$

### Logarithm (chain rule)

$$
\frac{\partial \ln g}{\partial \mathbf{X}}
  = \frac{1}{g}\,\frac{\partial g}{\partial \mathbf{X}}
$$

### Trace

$$
\frac{\partial\, \operatorname{tr}(\mathbf{A})}{\partial \mathbf{X}}
  = \boldsymbol{\delta} : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

**Special-case optimisations:**

- $\frac{\partial \mathbf{A}}{\partial \mathbf{X}} = \mathbb{I}$
  $\;\Longrightarrow\;$ result is $\boldsymbol{\delta}$ directly.
- $\frac{\partial \mathbf{A}}{\partial \mathbf{X}} = \mathbf{0}$
  $\;\Longrightarrow\;$ result is $\mathbf{0}$.

### Double contraction (dot product)

$$
\frac{\partial\, (\mathbf{A} : \mathbf{A})}{\partial \mathbf{X}}
  = 2\,\mathbf{A} : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

With the same identity-tensor optimisation as trace.

### Norm

$$
\frac{\partial\, \lVert\mathbf{A}\rVert}{\partial \mathbf{X}}
  = \frac{1}{\lVert\mathbf{A}\rVert}\;
    \mathbf{A} : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

### Determinant

$$
\frac{\partial\, \det(\mathbf{A})}{\partial \mathbf{X}}
  = \det(\mathbf{A})\;
    \mathbf{A}^{-\!\top}
    : \frac{\partial \mathbf{A}}{\partial \mathbf{X}}
$$

### Inner product to scalar

For $\varphi = \langle \mathbf{A},\, \mathbf{B} \rangle_{I_A, I_B}$ (full
contraction producing a scalar):

$$
\frac{\partial \varphi}{\partial \mathbf{X}}
  = \left\langle
      \frac{\partial \mathbf{A}}{\partial \mathbf{X}},\,
      \mathbf{B}
    \right\rangle_{I_A, I_B}
  + \left\langle
      \mathbf{A},\,
      \frac{\partial \mathbf{B}}{\partial \mathbf{X}}
    \right\rangle_{I_A, I_B}
$$

---

## API Reference

### Entry point

```cpp
#include <numsim_cas/core/diff.h>

// Tensor domain
#include <numsim_cas/tensor/tensor_diff.h>

// Tensor-to-scalar domain
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>

auto result = numsim::cas::diff(expr, arg);
```

The return type is always `expression_holder<tensor_expression>` for both
tensor and tensor-to-scalar differentiation with respect to a tensor argument.

### `tensor_differentiation`

```cpp
#include <numsim_cas/tensor/visitors/tensor_differentiation.h>

class tensor_differentiation final : public tensor_visitor_const_t {
public:
    using tensor_holder_t = expression_holder<tensor_expression>;

    explicit tensor_differentiation(tensor_holder_t const &arg);

    [[nodiscard]] tensor_holder_t apply(tensor_holder_t const &expr);
};
```

**Constructor** stores the differentiation variable and caches its dimension,
rank, and a Kronecker delta $\boldsymbol{\delta}$.

**`apply()`** resets internal state, dispatches to the appropriate
`operator()` via `accept(*this)`, and returns the result (or `tensor_zero` if
no dependency was found).

### `tensor_to_scalar_differentiation`

```cpp
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h>

class tensor_to_scalar_differentiation final
    : public tensor_to_scalar_visitor_const_t {
public:
    using tensor_holder_t  = expression_holder<tensor_expression>;
    using t2s_holder_t     = expression_holder<tensor_to_scalar_expression>;

    explicit tensor_to_scalar_differentiation(tensor_holder_t const &arg);

    [[nodiscard]] tensor_holder_t apply(t2s_holder_t const &expr);
};
```

Note: `apply()` accepts a **tensor-to-scalar** expression but returns a
**tensor** expression — this is the cross-domain nature of the derivative.

---

## Usage Examples

### Tensor self-derivative

```cpp
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_std.h>

using namespace numsim::cas;

auto [X, Y] = make_tensor_variable(
    std::tuple{"X", 3, 2},
    std::tuple{"Y", 3, 2});

auto d = diff(X, X);
// d is identity_tensor (rank 4)

auto d2 = diff(Y, X);
// d2 is tensor_zero (rank 4)

auto d3 = diff(X + Y, X);
// d3 is identity_tensor
```

### Trace derivative

```cpp
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

auto trY = trace(Y);
auto d = diff(trY, Y);
// d is kronecker_delta (I)

auto I = make_expression<kronecker_delta>(3);
assert(to_string(d) == to_string(I));
```

### Determinant derivative

```cpp
auto detY = det(Y);
auto d = diff(detY, Y);
// d is det(Y) * inv(trans(Y)) : identity_tensor
// i.e., det(Y) * inv(Y^T) contracted with the identity
```

### Chain rule: log(trace)

```cpp
auto f = log(trace(Y));
auto d = diff(f, Y);
// d = (1/tr(Y)) * I  via the chain rule
```

### Dot product derivative

```cpp
auto f = dot(Y);        // Y : Y
auto d = diff(f, Y);    // 2 * Y
assert(to_string(d) == to_string(2 * Y));
```

---

## File Reference

### Headers

| File | Purpose |
|------|---------|
| `core/diff.h` | `diff_fn` CPO definition |
| `tensor/tensor_diff.h` | `tag_invoke` for `diff(tensor, tensor)` |
| `tensor/visitors/tensor_differentiation.h` | Visitor class (20 node handlers) |
| `tensor_to_scalar/tensor_to_scalar_diff.h` | `tag_invoke` for `diff(t2s, tensor)` |
| `tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h` | Visitor class (13 node handlers) |

### Source files

| File | Purpose |
|------|---------|
| `tensor/visitors/tensor_differentiation.cpp` | 8 complex/cross-domain `operator()` implementations |
| `tensor_to_scalar/visitors/tensor_to_scalar_differentiation.cpp` | All 13 `operator()` implementations |

### Tests

| File | Coverage |
|------|----------|
| `tests/TensorDifferentiationTest.h` | Variable, addition, negation, scalar-mul, zero, constants, pow (8 tests) |
| `tests/TensorToScalarDifferentiationTest.h` | Trace, dot, norm, det, neg, add, log, zero, one, state-reset (11 tests) |

---

## Node Coverage Matrix

### Tensor nodes (20/20)

| # | Node | Rule | Impl |
|---|------|------|------|
| 1 | `tensor` | identity or zero | `.h` |
| 2 | `tensor_add` | sum rule | `.h` |
| 3 | `tensor_mul` | product rule | `.cpp` |
| 4 | `tensor_pow` | creates `tensor_power_diff` | `.h` |
| 5 | `tensor_power_diff` | contracts with $\partial\mathbf{A}/\partial\mathbf{X}$ | `.cpp` |
| 6 | `tensor_negative` | $-\partial\mathbf{A}/\partial\mathbf{X}$ | `.h` |
| 7 | `inner_product_wrapper` | product rule + index shift | `.cpp` |
| 8 | `basis_change_imp` | extended permutation | `.cpp` |
| 9 | `outer_product_wrapper` | product rule (outer) | `.cpp` |
| 10 | `kronecker_delta` | $\mathbf{0}$ | `.h` |
| 11 | `simple_outer_product` | product rule (outer, n-ary) | `.cpp` |
| 12 | `tensor_symmetry` | symmetrised derivative | `.h` |
| 13 | `tensor_deviatoric` | deviatoric projection | `.h` |
| 14 | `tensor_volumetric` | volumetric projection | `.h` |
| 15 | `tensor_inv` | $-\mathbf{A}^{-1}\,d\mathbf{A}\,\mathbf{A}^{-1}$ | `.cpp` |
| 16 | `tensor_zero` | $\mathbf{0}$ | `.h` |
| 17 | `tensor_projector` | $\mathbf{0}$ | `.h` |
| 18 | `identity_tensor` | $\mathbf{0}$ | `.h` |
| 19 | `tensor_scalar_mul` | $c \cdot \partial\mathbf{A}/\partial\mathbf{X}$ | `.h` |
| 20 | `tensor_to_scalar_with_tensor_mul` | $f\,d\mathbf{A} + \mathbf{A}\otimes df$ | `.cpp` |

### Tensor-to-scalar nodes (13/13)

| # | Node | Rule | Impl |
|---|------|------|------|
| 1 | `tensor_trace` | $\boldsymbol{\delta} : d\mathbf{A}/d\mathbf{X}$ | `.cpp` |
| 2 | `tensor_dot` | $2\,\mathbf{A} : d\mathbf{A}/d\mathbf{X}$ | `.cpp` |
| 3 | `tensor_det` | $\det(\mathbf{A})\;\mathbf{A}^{-\top}\!: d\mathbf{A}/d\mathbf{X}$ | `.cpp` |
| 4 | `tensor_norm` | $(\mathbf{A}:d\mathbf{A}/d\mathbf{X})/\lVert\mathbf{A}\rVert$ | `.cpp` |
| 5 | `tensor_to_scalar_negative` | $-d\varphi/d\mathbf{X}$ | `.cpp` |
| 6 | `tensor_to_scalar_add` | sum rule | `.cpp` |
| 7 | `tensor_to_scalar_mul` | product rule | `.cpp` |
| 8 | `tensor_to_scalar_pow` | generalised power rule | `.cpp` |
| 9 | `tensor_inner_product_to_scalar` | product rule | `.cpp` |
| 10 | `tensor_to_scalar_zero` | $\mathbf{0}$ | `.cpp` |
| 11 | `tensor_to_scalar_one` | $\mathbf{0}$ | `.cpp` |
| 12 | `tensor_to_scalar_log` | $(1/g)\,dg/d\mathbf{X}$ | `.cpp` |
| 13 | `tensor_to_scalar_scalar_wrapper` | $\mathbf{0}$ | `.cpp` |
