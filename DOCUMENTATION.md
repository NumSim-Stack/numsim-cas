# numsim-cas: Deep Fundamental Documentation

> **Version**: 1.0.0
> **Branch**: `move_to_virtual`
> **Language**: C++23

---

## Table of Contents

1. [Introduction & Motivation](#1-introduction--motivation)
2. [Foundational Concepts](#2-foundational-concepts)
3. [Core Layer](#3-core-layer)
4. [Scalar Domain](#4-scalar-domain)
5. [Tensor Domain](#5-tensor-domain)
6. [Tensor-to-Scalar Domain](#6-tensor-to-scalar-domain)
7. [Simplification Engine](#7-simplification-engine)
8. [Differentiation System](#8-differentiation-system)
9. [Evaluation System](#9-evaluation-system)
10. [Projection Tensor Algebra](#10-projection-tensor-algebra)
11. [Hash System & Canonical Ordering](#11-hash-system--canonical-ordering)
12. [Cross-Domain Interactions](#12-cross-domain-interactions)
13. [Build System & Dependencies](#13-build-system--dependencies)
14. [File Reference](#14-file-reference)
15. [Glossary](#15-glossary)

---

## 1. Introduction & Motivation

### 1.1 What is numsim-cas?

numsim-cas is a **Computer Algebra System** (CAS) implemented in C++23, designed specifically for **symbolic tensor calculus** in the context of computational mechanics. It enables:

- Building symbolic expressions involving scalars and tensors
- Automatic algebraic simplification (constant folding, like-term merging, identity laws)
- Symbolic differentiation (chain rule, product rule, Leibniz rule)
- Numerical evaluation using the [tmech](https://github.com/petlenz/tmech) tensor library
- Projection tensor algebra (symmetric, deviatoric, volumetric, skew decompositions)

### 1.2 Design Philosophy

1. **Type safety**: Three distinct expression domains prevent mixing incompatible types
2. **Immutability**: Expression nodes are never modified after construction -- all transformations produce new nodes
3. **Eager simplification**: Every operation simplifies immediately, producing canonical forms
4. **Extensibility**: The tag_invoke CPO pattern allows adding new operations without modifying existing code
5. **Performance**: Shared pointer semantics enable subexpression sharing; hash-based lookup enables O(1) simplification checks

### 1.3 Domain Model

The library models three mathematical domains connected by typed bridges:

```
┌──────────────────────────────────────────────────────────────┐
│                        Core Layer                            │
│  expression, expression_holder, visitor_base, n_ary_tree,    │
│  domain_traits, tag_invoke, hash_functions, scalar_number    │
└────────────────────┬───────────────┬──────────────┬──────────┘
                     │               │              │
         ┌───────────▼──┐   ┌───────▼───────┐  ┌──▼────────────────┐
         │    Scalar     │   │    Tensor     │  │  Tensor-to-Scalar │
         │   Domain      │   │    Domain     │  │     Domain        │
         │               │   │               │  │                   │
         │ x, y, z       │   │ A, B, C       │  │ trace(A), det(A)  │
         │ sin, cos, pow │   │ sym, dev, vol │  │ norm(A), dot(A,B) │
         │ +, -, *, /    │   │ inner, outer  │  │ +, -, *, pow      │
         └───────┬───────┘   └───────┬───────┘  └──┬────────────────┘
                 │                   │              │
                 └────────connects───┘──────────────┘
                   via scalar_mul, scalar_wrapper,
                   tensor_to_scalar_with_tensor_mul
```

### 1.4 Target Audience

The library targets researchers and engineers in computational mechanics who need to:
- Derive material tangent tensors symbolically
- Implement constitutive models with automatic differentiation
- Verify hand-derived formulas against symbolic computation
- Generate code for finite element solvers

---

## 2. Foundational Concepts

### 2.1 Expression Trees as DAGs

Every symbolic expression is represented as a **Directed Acyclic Graph** (DAG) of nodes. Each node is either:
- A **leaf** (variable, constant, zero, one)
- A **unary operation** (negation, sin, cos, transpose, ...)
- A **binary operation** (pow, rational, scalar_mul, ...)
- An **n-ary operation** (add, mul -- variadic number of children)

Nodes are immutable after construction. Shared subexpressions are represented by multiple references to the same node (via `shared_ptr`), forming a DAG rather than a tree:

```
Expression:  x^2 + 2*x + 1

DAG:           add
              / | \
          pow  mul  one
          /\   /\
         x  2 2  x     ← same 'x' node shared
```

### 2.2 The expression_holder Wrapper

All expression nodes are accessed through `expression_holder<Base>`, a RAII wrapper around `std::shared_ptr<node_type>`:

```cpp
expression_holder<scalar_expression> x = make_scalar_variable("x");

// Access the underlying node:
auto& node = x.get();              // returns scalar_expression&
auto& concrete = x.get<scalar>();  // downcast to concrete type

// Validity check:
if (x.is_valid()) { ... }

// Automatic operator dispatch:
auto f = x + x;  // triggers tag_invoke(add_fn, x, x)
```

The wrapper provides:
- **RAII**: Automatic memory management via shared_ptr
- **Type safety**: `expression_holder<scalar_expression>` cannot hold a tensor node
- **Operator overloads**: `+`, `-`, `*`, `/`, unary `-` delegate to CPOs
- **Compound assignment**: `+=`, `*=` for in-place modification (creates new node)

### 2.3 Virtual Visitor Pattern

Unlike variant-based CAS implementations, numsim-cas uses the **virtual visitor pattern**. Each expression domain defines a visitor base class with pure virtual `operator()` overloads for every node type:

```cpp
// Generated from the node list macro:
class scalar_visitor_const_t {
public:
    virtual void operator()(scalar const&) = 0;
    virtual void operator()(scalar_zero const&) = 0;
    virtual void operator()(scalar_one const&) = 0;
    virtual void operator()(scalar_constant const&) = 0;
    virtual void operator()(scalar_add const&) = 0;
    virtual void operator()(scalar_mul const&) = 0;
    virtual void operator()(scalar_pow const&) = 0;
    // ... one per node type (21 total for scalar)
};
```

Each node implements `accept(visitor)` via CRTP:

```cpp
template <typename Base, typename Derived, typename... Types>
class visitable_impl : public visitable<Base, Types...> {
    void accept(visitor_const<Types...>& v) const override {
        v(static_cast<Derived const&>(*this));  // double dispatch
    }
};
```

This design was chosen over `std::variant` because:
- Adding new node types doesn't change existing visitor overload sets
- Virtual dispatch has predictable performance
- The visitor pattern is well-suited to the many-operations-few-types shape of CAS code

### 2.4 Tag-Invoke CPO Pattern

Operators (`+`, `-`, `*`, `/`) are implemented using the **Customization Point Object** (CPO) pattern based on `tag_invoke`:

```cpp
// 1. Define a tag type:
struct add_fn {
    template <class L, class R>
    requires tag_invocable<add_fn, L&&, R&&>
    constexpr auto operator()(L&& l, R&& r) const {
        return tag_invoke(*this, std::forward<L>(l), std::forward<R>(r));
    }
};
inline constexpr add_fn binary_add{};

// 2. Global operator delegates to CPO:
template <class L, class R>
requires cas_binary_op<L, R>
auto operator+(L&& l, R&& r) {
    return detail::binary_add(std::forward<L>(l), std::forward<R>(r));
}

// 3. Each domain implements via ADL:
// In scalar_operators.h:
expression_holder<scalar_expression>
tag_invoke(add_fn, scalar_expr auto&& lhs, scalar_expr auto&& rhs) {
    // ... simplification logic ...
}
```

This pattern allows each domain to customize behavior for its own types without modifying the core operator infrastructure.

### 2.5 Domain Traits

The `domain_traits<Domain>` template provides type-level information about each expression domain:

```cpp
template <> struct domain_traits<scalar_expression> {
    using expr_holder_t   = expression_holder<scalar_expression>;
    using add_type        = scalar_add;
    using mul_type        = scalar_mul;
    using pow_type        = scalar_pow;
    using negative_type   = scalar_negative;
    using zero_type       = scalar_zero;
    using one_type        = scalar_one;
    using constant_type   = scalar_constant;
    using symbol_type     = scalar;

    static expr_holder_t zero();
    static expr_holder_t one();
    static std::optional<scalar_number> try_numeric(expr_holder_t const&);
    static expr_holder_t make_constant(scalar_number const&);
};
```

Generic simplification algorithms are parameterized by these traits, enabling code reuse across domains.

---

## 3. Core Layer

### 3.1 expression (Base Class)

`expression` is the root of the node hierarchy. Every node in every domain ultimately inherits from it.

**Key members:**
- `m_hash_value` (mutable, lazily computed): Cached hash for O(1) comparison fast-path
- `m_assumption`: Numeric assumptions (positive, negative, integer, etc.)

**Key methods:**
- `hash_value()`: Returns cached hash, computing it on first call via `update_hash_value()`
- `id()`: Returns a unique type ID per concrete node class
- `operator==`: Hash-first comparison, then deep structural comparison
- `operator<`: Total ordering for canonical print output

**Comparison protocol:**
```
a == b  ⟺  a.hash == b.hash  AND  a.equals_same_type(b)
a < b   ⟺  a.hash < b.hash   OR  (a.hash == b.hash AND a.less_than_same_type(b))
```

### 3.2 Node Base Templates

#### unary_op<ThisBase, ExprBase>
Holds one child expression. Used for: negation, sin, cos, tan, transpose, inverse, etc.

```cpp
template <typename ThisBase, typename ExprBase>
class unary_op : public ThisBase {
    expression_holder<ExprBase> m_expr;
public:
    auto const& expr() const { return m_expr; }
};
```

#### binary_op<ThisBase, BaseLHS, BaseRHS>
Holds two child expressions. Used for: pow, scalar_mul, rational, tensor products, etc.

```cpp
template <typename ThisBase, typename BaseLHS, typename BaseRHS>
class binary_op : public ThisBase {
    expression_holder<BaseLHS> m_lhs;
    expression_holder<BaseRHS> m_rhs;
public:
    auto const& expr_lhs() const { return m_lhs; }
    auto const& expr_rhs() const { return m_rhs; }
};
```

Note: `BaseLHS` and `BaseRHS` can be different types, enabling cross-domain operations (e.g., `tensor_scalar_mul` has `scalar_expression` LHS and `tensor_expression` RHS).

#### n_ary_tree<Base>
Hash-map-based storage for commutative n-ary operations (addition, multiplication).

```cpp
template <typename Base>
class n_ary_tree : public Base {
    expr_ordered_map<expr_holder_t> m_symbol_map;  // hash -> child expr
    expr_holder_t m_coeff;                          // numeric coefficient
};
```

Children are stored in an ordered map keyed by hash value. The coefficient is stored separately and excluded from the hash. This enables `3*x` and `5*x` to have the same hash as `x`, which is critical for like-term merging.

**Hash computation:**
1. Start with the node type ID
2. Collect all child hashes into a buffer
3. Sort the buffer (commutativity: `a+b` and `b+a` hash the same)
4. `hash_combine` each sorted hash into the seed

#### n_ary_vector<Base>
Vector-based storage for non-commutative n-ary operations (tensor products, inner products).

```cpp
template <typename Base>
class n_ary_vector : public Base {
    expr_vector<expr_holder_t> m_data;  // ordered list
    expr_holder_t m_coeff;               // coefficient
};
```

Unlike `n_ary_tree`, the hash does NOT sort children -- order matters.

#### symbol_base<BaseExpr>
Base for leaf nodes representing named variables.

```cpp
template <typename BaseExpr>
class symbol_base : public BaseExpr {
    std::string m_name;
public:
    auto const& name() const { return m_name; }
};
```

Hash is based solely on the variable name.

### 3.3 scalar_number

A type-erasing numeric wrapper:

```cpp
class scalar_number {
    std::variant<std::int64_t, double, std::complex<double>> v_;
public:
    // Arithmetic with automatic type promotion
    friend scalar_number operator+(scalar_number, scalar_number);
    friend scalar_number operator*(scalar_number, scalar_number);
    // Comparison
    friend bool operator==(scalar_number, scalar_number);
    friend bool operator<(scalar_number, scalar_number);
};
```

Integers stay exact (`int64_t`), promoted to `double` when mixed with floats, and to `complex<double>` when mixed with complex.

### 3.4 Assumptions

The assumption system attaches metadata to expressions:

```cpp
// Numeric assumptions
assume(x, positive{});
assume(x, integer{});

// Queries
bool is_positive(expr);
bool is_negative(expr);
bool is_nonnegative(expr);

// Implications: assume(x, positive{}) also implies nonzero{} and nonnegative{}
```

Assumptions are used by construction-time simplification:
- `abs(x)` with `positive` assumption returns `x`
- `sign(x)` with `positive` assumption returns `1`
- `sqrt(pow(x, 2))` with `nonnegative` assumption returns `x`

### 3.5 Sequences (Index Notation)

The `sequence` class represents ordered index lists for tensor operations:

```cpp
sequence s{3, 4};  // User input: 1-based indices 3 and 4
// Internally stored as {2, 3} (0-based)

// Operations:
auto c = concat(seq1, seq2);         // concatenate
auto [a, b] = split(seq, n);         // split at position n
auto p = permute(seq, perm);          // reorder by permutation
auto inv = invert_perm(perm);         // inverse permutation
```

**Convention**: All public APIs accept 1-based indices (mathematical notation). Internal storage is 0-based (C++ convention).

---

## 4. Scalar Domain

### 4.1 Node Types

| Node | Base | Description |
|------|------|-------------|
| `scalar` | `symbol_base` | Named variable (x, y, z) |
| `scalar_zero` | leaf | Additive identity (0) |
| `scalar_one` | leaf | Multiplicative identity (1) |
| `scalar_constant` | leaf | Numeric constant (wraps `scalar_number`) |
| `scalar_add` | `n_ary_tree` | Commutative addition (coefficient + children) |
| `scalar_mul` | `n_ary_tree` | Commutative multiplication (coefficient * children) |
| `scalar_negative` | `unary_op` | Unary negation (-x) |
| `scalar_pow` | `binary_op` | Exponentiation (x^n) |
| `scalar_rational` | `binary_op` | Rational expression (numerator/denominator) |
| `scalar_sin` | `unary_op` | Sine function |
| `scalar_cos` | `unary_op` | Cosine function |
| `scalar_tan` | `unary_op` | Tangent function |
| `scalar_asin` | `unary_op` | Arcsine function |
| `scalar_acos` | `unary_op` | Arccosine function |
| `scalar_atan` | `unary_op` | Arctangent function |
| `scalar_sqrt` | `unary_op` | Square root |
| `scalar_exp` | `unary_op` | Exponential function |
| `scalar_log` | `unary_op` | Natural logarithm |
| `scalar_abs` | `unary_op` | Absolute value |
| `scalar_sign` | `unary_op` | Sign function (-1, 0, or 1) |
| `scalar_named_expression` | `unary_op` | Named wrapper (user-defined functions) |

### 4.2 Creating Expressions

```cpp
using namespace numsim::cas;

// Variables
auto [x, y, z] = make_scalar_variable("x", "y", "z");

// Constants
auto [_1, _2, _3] = make_scalar_constant(1, 2, 3);

// Special constants
auto zero = get_scalar_zero();
auto one  = get_scalar_one();

// Expressions (simplify automatically)
auto f = pow(x, _2) + _2 * x * y + pow(y, _2);
auto g = sin(x) + cos(y);
auto h = exp(log(x));  // simplifies to x at construction time
```

### 4.3 Operations

```cpp
// Arithmetic (delegates to tag_invoke CPOs)
auto sum  = f + g;      // addition
auto diff = f - g;      // subtraction (implemented as f + (-g))
auto prod = f * g;      // multiplication
auto quot = f / g;      // division (implemented as f * pow(g, -1))
auto neg  = -f;         // negation

// Functions (construction-time simplification)
using std::sin, std::cos, std::pow, std::exp, std::log, std::sqrt;
auto s = sin(x);
auto c = cos(x);
auto p = pow(x, _2);
auto e = exp(x);
auto l = log(x);
auto r = sqrt(x);

// Differentiation
auto df_dx = diff(f, x);

// Substitution
auto result = substitute(f, x, _2 + y);  // replace x with 2+y

// Evaluation
scalar_evaluator<double> ev;
ev.set(x, 3.0);
ev.set(y, 2.0);
double value = ev.apply(f);
```

### 4.4 Internal Representation

An `n_ary_tree`-based add node stores:

```
scalar_add {
    coefficient: scalar_constant(5)    // the numeric part
    children: {
        hash(x) -> x,                 // variable terms
        hash(y) -> scalar_mul{coeff: 3, children: {y}},  // 3*y
        hash(pow(z,2)) -> pow(z, 2)   // non-linear terms
    }
}
```

This represents `5 + x + 3*y + z^2`. The hash-map enables O(1) lookup when adding a new term -- if it matches an existing child's hash, the coefficients are merged.

---

## 5. Tensor Domain

### 5.1 Node Types

| Node | Base | Description |
|------|------|-------------|
| `tensor` | `symbol_base` | Named tensor variable (A, B, C) with dim and rank |
| `tensor_zero` | leaf | Zero tensor of given dim and rank |
| `identity_tensor` | leaf | Identity tensor (I for rank 2, I_ijkl for rank 4) |
| `kronecker_delta` | leaf | Kronecker delta (always rank 2) |
| `tensor_projector` | leaf | Projection tensor (P_sym, P_dev, P_vol, P_skew) |
| `tensor_add` | `n_ary_tree` | Commutative tensor addition |
| `tensor_mul` | `n_ary_vector` | Non-commutative tensor product (A*B) |
| `tensor_pow` | `binary_op` | Tensor power (A^n for integer n) |
| `tensor_negative` | `unary_op` | Tensor negation (-A) |
| `tensor_scalar_mul` | `binary_op` | Scalar-tensor product (s*A) |
| `tensor_inv` | `unary_op` | Matrix inverse (A^{-1}) |
| `inner_product_wrapper` | binary with indices | Index contraction (A:B) |
| `outer_product_wrapper` | binary with indices | Tensor product (A ⊗ B) |
| `basis_change_imp` | unary with indices | Index permutation (transpose) |
| `simple_outer_product` | n-ary | Iterated outer product |
| `tensor_to_scalar_with_tensor_mul` | binary | Cross-domain: T2S * Tensor |

### 5.2 Tensor Properties

Every tensor expression carries:
- **dim**: Spatial dimension (1, 2, or 3)
- **rank**: Tensor rank (0=scalar, 1=vector, 2=matrix, 4=fourth-order, etc.)
- **space** (optional): Algebraic space classification

```cpp
auto [A] = make_tensor_variable(std::tuple{"A", 3, 2});
// A is a rank-2 tensor in 3D (i.e., a 3x3 matrix)

A.get().dim();   // 3
A.get().rank();  // 2
A.get().space(); // std::nullopt (general, no constraints)
```

### 5.3 Tensor Spaces

The `tensor_space` classifies tensors by symmetry and trace properties:

```cpp
struct tensor_space {
    std::variant<General, Symmetric, Skew, Young> perm;     // permutation symmetry
    std::variant<AnyTraceTag, VolumetricTag, DeviatoricTag,
                 HarmonicTag, PartialTraceTag> trace;       // trace constraint
};
```

**Permutation spaces:**
- `General`: No symmetry constraints
- `Symmetric`: A_ij = A_ji (with specific index permutations)
- `Skew`: A_ij = -A_ji
- `Young`: Young tableau symmetry (for higher ranks)

**Trace spaces:**
- `AnyTraceTag`: No trace constraint
- `VolumetricTag`: Proportional to identity (A = (1/d)tr(A)*I)
- `DeviatoricTag`: Trace-free (tr(A) = 0)
- `HarmonicTag`: Both symmetric and trace-free
- `PartialTraceTag`: Partial traces vanish

**Space join semantics** (used when adding tensors):
- `Symmetric + Symmetric -> Symmetric` (preserved)
- `Symmetric + General -> General` (widened)
- `Volumetric + Deviatoric -> Symmetric` (combined via projectors)
- Incompatible spaces -> cleared (no constraint)

### 5.4 Creating Tensor Expressions

```cpp
using namespace numsim::cas;

// Variables
auto [A, B] = make_tensor_variable(
    std::tuple{"A", 3, 2}, std::tuple{"B", 3, 2});

auto [C] = make_tensor_variable(std::tuple{"C", 3, 4});  // rank-4

// Special tensors
auto I = make_expression<identity_tensor>(3, 2);     // 3D rank-2 identity
auto delta = make_expression<kronecker_delta>(3);     // Kronecker delta
auto Z = make_expression<tensor_zero>(3, 2);          // Zero tensor

// Arithmetic
auto sum   = A + B;       // tensor addition
auto diff  = A - B;       // tensor subtraction
auto neg   = -A;          // negation
auto scale = x * A;       // scalar-tensor multiplication
auto power = pow(A, _2);  // tensor power (A*A via contraction)
```

### 5.5 Tensor Products and Contractions

```cpp
using seq = sequence;

// Inner product (contraction)
auto C = inner_product(A, seq{2}, B, seq{1});  // A_ij * B_jk = C_ik

// Full contraction (double inner product)
auto D = inner_product(A, seq{1,2}, B, seq{1,2});  // A:B (scalar result in tensor domain)

// Outer product
auto E = otimes(A, B);        // A_ij * B_kl = E_ijkl (rank 4)
auto F = otimesu(A, B);       // A_ik * B_jl (upper product)
auto G = otimesl(A, B);       // A_il * B_jk (lower product)

// Index permutation / transpose
auto At = trans(A);            // A^T (equivalent to basis_change with {2,1})
auto Ap = permute_indices(C, seq{2,1,4,3});  // arbitrary index permutation

// Matrix inverse
auto Ainv = inv(A);            // A^{-1}
```

### 5.6 Projection Functions

```cpp
// Projections (construction-time simplification)
auto As = sym(A);    // symmetric part:  P_sym : A
auto Ad = dev(A);    // deviatoric part: P_devi : A
auto Av = vol(A);    // volumetric part: P_vol : A
auto Ak = skew(A);   // skew part:       P_skew : A

// Standalone projectors (rank-4 tensors)
auto Ps = P_sym(3);    // symmetric projector in 3D
auto Pd = P_devi(3);   // deviatoric projector
auto Pv = P_vol(3);    // volumetric projector
auto Pk = P_skew(3);   // skew projector

// Projector algebra (at construction time):
dev(dev(A));        // -> dev(A)        (idempotence)
vol(dev(A));        // -> 0             (orthogonality)
sym(skew(A));       // -> 0             (orthogonality)
vol(A) + dev(A);    // -> sym(A)        (decomposition)
sym(A) + skew(A);   // -> A             (complete decomposition)

// Assumptions enable further simplification:
assume_symmetric(A);
sym(A);             // -> A             (already symmetric)
skew(A);            // -> 0             (symmetric => no skew part)
```

---

## 6. Tensor-to-Scalar Domain

### 6.1 Purpose

The tensor-to-scalar domain bridges tensor expressions with scalar algebra. It represents operations that take tensors as input and produce scalars:

```
trace(A)  : rank-2 tensor -> scalar
det(A)    : rank-2 tensor -> scalar
norm(A)   : rank-2 tensor -> scalar
dot(A, B) : two tensors   -> scalar (dot product)
```

These scalar results can then participate in scalar arithmetic and be used as coefficients in tensor expressions.

### 6.2 Node Types

| Node | Base | Description |
|------|------|-------------|
| `tensor_to_scalar_zero` | leaf | Additive identity (0) |
| `tensor_to_scalar_one` | leaf | Multiplicative identity (1) |
| `tensor_to_scalar_scalar_wrapper` | `unary_op` | Wraps a `scalar_expression` for use in T2S |
| `tensor_trace` | `unary_op<..., tensor_expression>` | Trace operation |
| `tensor_dot` | `unary_op<..., tensor_expression>` | Self dot product (A:A) |
| `tensor_det` | `unary_op<..., tensor_expression>` | Determinant |
| `tensor_norm` | `unary_op<..., tensor_expression>` | Frobenius norm (sqrt(A:A)) |
| `tensor_to_scalar_add` | `n_ary_tree` | Addition |
| `tensor_to_scalar_mul` | `n_ary_tree` | Multiplication |
| `tensor_to_scalar_pow` | `binary_op` | Power |
| `tensor_to_scalar_negative` | `unary_op` | Negation |
| `tensor_to_scalar_log` | `unary_op` | Natural logarithm |
| `tensor_inner_product_to_scalar` | binary with indices | Indexed inner product -> scalar |

### 6.3 Creating T2S Expressions

```cpp
using namespace numsim::cas;

auto [A, B] = make_tensor_variable(
    std::tuple{"A", 3, 2}, std::tuple{"B", 3, 2});
auto [x] = make_scalar_variable("x");

// T2S functions
auto tr = trace(A);           // tr(A)
auto d  = det(A);             // det(A)
auto n  = norm(A);            // ||A|| (Frobenius norm)
auto dp = dot(A);             // A:A (self dot product)
auto ip = dot_product(A, seq{1,2}, B, seq{1,2});  // A:B

// Mixed arithmetic (T2S with scalars)
auto f = x * trace(A) + det(B);   // scalar * T2S + T2S
auto g = pow(trace(A), 2);        // T2S power

// Construction-time simplifications
trace(make_expression<tensor_zero>(3, 2));     // -> 0
trace(make_expression<kronecker_delta>(3));     // -> 3 (dimension)
det(make_expression<kronecker_delta>(3));       // -> 1
norm(make_expression<tensor_zero>(3, 2));       // -> 0
```

### 6.4 The Scalar Wrapper

The `tensor_to_scalar_scalar_wrapper` node wraps a plain scalar expression so it can participate in T2S arithmetic:

```cpp
// When you write:
auto f = x + trace(A);

// Internally, x is wrapped:
// tensor_to_scalar_add {
//     children: {
//         tensor_to_scalar_scalar_wrapper(x),
//         tensor_trace(A)
//     }
// }
```

This wrapping happens automatically in the operator overloads. The simplifiers know how to unwrap scalars for numeric operations and re-wrap the results.

---

## 7. Simplification Engine

### 7.1 Architecture

Simplification happens immediately when an operation is performed. The call chain for `a + b`:

```
operator+(a, b)
  └─> detail::binary_add(a, b)       // CPO dispatch
       └─> tag_invoke(add_fn, a, b)   // domain-specific
            └─> add_base(a, b)         // dispatcher
                 ├─ Quick checks: a+0->a, a+(-a)->0
                 ├─ a.accept(add_base)  // visitor dispatch on LHS type
                 │   └─> add_base.dispatch(concrete_lhs)
                 │        └─ Creates specialized visitor for LHS type
                 │        └─ rhs.accept(specialized_visitor)
                 │             └─> specialized_visitor.dispatch(concrete_rhs)
                 │                  └─ Apply specific simplification rule
                 └─ If no special rule: get_default()  // build new add node
```

### 7.2 Dispatcher Hierarchy (Addition Example)

```
add_base (main dispatcher)
│
├── dispatch(scalar_zero)      → return rhs (additive identity)
├── dispatch(scalar_one)       → create one_add visitor
├── dispatch(scalar_constant)  → create constant_add visitor
├── dispatch(scalar_add)       → create n_ary_add visitor (flatten)
├── dispatch(scalar_mul)       → create n_ary_mul_add visitor
├── dispatch(scalar)           → create symbol_add visitor
├── dispatch(scalar_negative)  → create negative_add visitor
└── dispatch(other)            → swap operands if add, else get_default()
```

Each specialized visitor then dispatches on the RHS type:

```
constant_add (LHS is constant)
│
├── dispatch(scalar_constant)  → fold: c1 + c2
├── dispatch(scalar_zero)      → return c1
├── dispatch(scalar_one)       → c1 + 1
├── dispatch(scalar_add)       → merge constant into add's coefficient
├── dispatch(scalar_negative)  → c1 + (-x) = c1 - x
└── dispatch(other)            → build new add
```

### 7.3 Generic Algorithms (Domain Traits)

The core simplifier algorithms in `core/simplifier/` are parameterized by domain traits:

```cpp
template <typename Traits>
class add_dispatch {
    using expr_holder_t = typename Traits::expr_holder_t;
    using add_type      = typename Traits::add_type;
    using mul_type      = typename Traits::mul_type;
    using zero_type     = typename Traits::zero_type;
    using constant_type = typename Traits::constant_type;

    expr_holder_t get_default() {
        // Build new add node using domain types
        auto result = make_expression<add_type>(...);
        auto& tree = result.template get<add_type>();
        // ... populate tree ...
        return result;
    }
};
```

Each domain provides thin wrapper classes:

```cpp
// Scalar domain (scalar_simplifier_add.h):
class scalar_add_dispatch : public scalar_visitor_return_expr_t,
                             public add_dispatch<domain_traits<scalar_expression>> {
    // Uses the node list macro to declare dispatch methods
};
```

### 7.4 Key Simplification Rules

**Addition:**
| Rule | Example |
|------|---------|
| Additive identity | `x + 0 -> x` |
| Same expression | `x + x -> 2*x` |
| Constant folding | `3 + 5 -> 8` |
| Flattening | `(a+b) + c -> a+b+c` |
| Coefficient merge | `3*x + 5*x -> 8*x` |
| Cancellation | `x + (-x) -> 0` |
| Negative merge | `(-a) + (-b) -> -(a+b)` |

**Multiplication:**
| Rule | Example |
|------|---------|
| Multiplicative identity | `x * 1 -> x` |
| Annihilator | `x * 0 -> 0` |
| Same expression | `x * x -> pow(x, 2)` |
| Constant folding | `3 * 5 -> 15` |
| Flattening | `(a*b) * c -> a*b*c` |
| Power merge | `x * pow(x, n) -> pow(x, n+1)` |
| Coefficient extract | `(3*x) * (5*y) -> 15*x*y` |

**Exponentiation:**
| Rule | Example |
|------|---------|
| Identity | `x^1 -> x` |
| Zero | `x^0 -> 1` |
| Power of power | `pow(pow(x, a), b) -> pow(x, a*b)` |
| Power of product | `pow(x*y, n) -> pow(x,n)*pow(y,n)` (integer n) |
| Inverse cancellation | `x * pow(x, -1) -> 1` |
| Division | `x / y -> x * pow(y, -1)` |

**Construction-time (functions):**
| Rule | Example |
|------|---------|
| sin(0) | `-> 0` |
| cos(0) | `-> 1` |
| tan(0) | `-> 0` |
| sin(asin(x)) | `-> x` |
| cos(acos(x)) | `-> x` |
| exp(0) | `-> 1` |
| exp(log(x)) | `-> x` |
| log(1) | `-> 0` |
| log(exp(x)) | `-> x` |
| sqrt(0) | `-> 0` |
| sqrt(1) | `-> 1` |
| abs(x) if x>0 | `-> x` |
| sign(x) if x>0 | `-> 1` |

---

## 8. Differentiation System

### 8.1 Architecture

Differentiation is implemented as a visitor that walks the expression tree and applies derivative rules. Each domain has its own differentiation visitor:

- `scalar_differentiation`: d(scalar)/d(scalar) -> scalar
- `tensor_differentiation`: d(tensor)/d(tensor) -> tensor (higher rank)
- `tensor_to_scalar_differentiation`: d(T2S)/d(tensor) -> tensor

### 8.2 CPO Interface

```cpp
// Scalar differentiation
auto df = diff(f, x);  // d(f)/d(x)

// Tensor differentiation
auto dA = diff(expr, A);  // d(expr)/d(A), result rank = expr.rank + A.rank

// T2S to tensor differentiation
auto dT = diff(trace(A), A);  // d(trace(A))/d(A) = I
```

The `diff` CPO dispatches via `tag_invoke`:
```cpp
struct diff_fn {
    template <class ExprBase, class ArgBase>
    auto operator()(expression_holder<ExprBase> const& expr,
                    expression_holder<ArgBase> const& arg) const;
};
inline constexpr diff_fn diff{};
```

### 8.3 Scalar Derivative Rules

| Expression | Derivative |
|-----------|-----------|
| constant, zero, one | `0` |
| x (matches arg) | `1` |
| x (different) | `0` |
| u + v | `du + dv` |
| u * v | `u*dv + du*v` (product rule) |
| -u | `-du` |
| u^n (constant n) | `n * u^(n-1) * du` (chain rule) |
| u^v (general) | `u^(v-1) * (v' * log(u) * u + v * u')` |
| sin(u) | `cos(u) * du` |
| cos(u) | `-sin(u) * du` |
| tan(u) | `(1/cos(u)^2) * du` |
| asin(u) | `du / sqrt(1 - u^2)` |
| acos(u) | `-du / sqrt(1 - u^2)` |
| atan(u) | `du / (1 + u^2)` |
| exp(u) | `exp(u) * du` |
| log(u) | `du / u` |
| sqrt(u) | `du / (2*sqrt(u))` |
| abs(u) | `u/abs(u) * du` |
| sign(u) | `0` (distributional) |

### 8.4 Tensor Derivative Rules

Tensor differentiation produces higher-rank tensors:

| Expression | Derivative d/dA | Rank Change |
|-----------|----------------|-------------|
| A (matches arg) | I or P_space | +rank(A) |
| B (different) | 0 | +rank(A) |
| A + B | dA/dA + dB/dA | same |
| s * A | s * dA/dA | same |
| -A | -dA/dA | same |
| A^n | sum of A^(i-1) * dA/dA * A^(n-i) | same |
| inv(A) | -inv(A) * dA/dA * inv(A) | same |
| trans(A) | trans(dA/dA) | same |
| A : B | dA/dA : B + A : dB/dA | depends |

**Space-aware derivatives:**
When the differentiation variable has a tensor space, the identity tensor is replaced by the appropriate projector:
- `d(symmetric_A)/d(symmetric_A) = P_sym` (not I)
- `d(deviatoric_A)/d(deviatoric_A) = P_devi` (not I)

### 8.5 T2S Derivative Rules

| Expression | Derivative d/dA | Result |
|-----------|----------------|--------|
| trace(A) | I | identity tensor |
| det(A) | det(A) * inv(A)^T | cofactor matrix |
| dot(A) (A:A) | 2*A | tensor |
| norm(A) | A / norm(A) | tensor |

---

## 9. Evaluation System

### 9.1 Architecture

Evaluation transforms symbolic expressions into numeric values using the visitor pattern:

```cpp
// Scalar evaluation
scalar_evaluator<double> sev;
sev.set(x, 3.0);
sev.set(y, 2.0);
double result = sev.apply(f);

// Tensor evaluation
tensor_evaluator<double> tev;
auto A_data = make_tensor_data<double>(3, 2);  // 3D rank-2
A_data->data() = some_tmech_tensor;
tev.set(A, std::move(A_data));
auto result_data = tev.apply(expr);

// T2S evaluation
tensor_to_scalar_evaluator<double> t2sev;
t2sev.set(A, A_data);
double trace_val = t2sev.apply(trace(A));
```

### 9.2 Tensor Data Infrastructure

Tensor evaluation produces `tensor_data_base<ValueType>` objects that wrap `tmech::tensor<ValueType, Dim, Rank>`:

```cpp
template <typename ValueType, std::size_t Dim, std::size_t Rank>
class tensor_data : public tensor_data_base<ValueType> {
    tmech::tensor<ValueType, Dim, Rank> m_data;
};
```

Since `Dim` and `Rank` are template parameters but can vary at runtime, the evaluator uses a dispatch mechanism (`tensor_data_eval`) that tries all valid Dim/Rank combinations:

```cpp
// Pseudo-code for dispatch:
if (dim == 1 && rank == 2) evaluate<1, 2>(...);
else if (dim == 2 && rank == 2) evaluate<2, 2>(...);
else if (dim == 3 && rank == 2) evaluate<3, 2>(...);
// ... etc for rank 1, 3, 4
```

### 9.3 Evaluator Short-Circuits

The tensor evaluator includes optimized paths for common patterns:

- **Projector contractions**: `P:A` where P is a known projector and A is rank-2 directly calls `tmech::sym()`, `tmech::dev()`, `tmech::vol()`, or `tmech::skew()` instead of full rank-4 contraction
- **Identity tensor**: Rank-2 uses `tmech::eye`, rank-4 uses `tmech::otimesu(I, I)` (minor identity)
- **Zero tensor**: Returns zero-initialized data without computation

---

## 10. Projection Tensor Algebra

### 10.1 Motivation

In continuum mechanics, tensors are often decomposed into symmetric, deviatoric, volumetric, and skew-symmetric parts. This decomposition is performed by fourth-order projection tensors:

- **P_sym**: Projects onto symmetric part: `(P_sym:A)_ij = (A_ij + A_ji)/2`
- **P_skew**: Projects onto skew part: `(P_skew:A)_ij = (A_ij - A_ji)/2`
- **P_vol**: Projects onto volumetric part: `(P_vol:A)_ij = (1/d)*tr(A)*delta_ij`
- **P_devi**: Projects onto deviatoric part: `P_devi = P_sym - P_vol`

These satisfy important algebraic rules that the CAS exploits for simplification.

### 10.2 Projector Node

```cpp
class tensor_projector {
    std::size_t m_dim;           // spatial dimension
    std::size_t m_acts_on_rank;  // rank of input (2 for rank-2 tensors)
    tensor_space m_space;        // identifies which projector
};
```

Rank of the projector = 2 * acts_on_rank (e.g., rank 4 for rank-2 projectors).

Factory functions:
```cpp
P_sym(dim)          // Symmetric projector
P_skew(dim)         // Skew projector
P_vol(dim)          // Volumetric projector
P_devi(dim)         // Deviatoric projector
P_harm(dim, rank)   // Harmonic projector
```

### 10.3 Contraction Rules

When two projectors are contracted (`P_a : P_b`), the result is determined by:

```
enum class ContractionRule { Idempotent, Zero, LhsSubspace, RhsSubspace };
```

**Contraction table (P_a : P_b -> result):**

| P_a \ P_b | Sym | Skew | Vol | Dev |
|------------|-----|------|-----|-----|
| **Sym** | Sym (idem.) | 0 | Vol (rhs_sub) | Dev (rhs_sub) |
| **Skew** | 0 | Skew (idem.) | 0 | 0 |
| **Vol** | Vol (lhs_sub) | 0 | Vol (idem.) | 0 |
| **Dev** | Dev (lhs_sub) | 0 | 0 | Dev (idem.) |

- **Idempotent**: P:P -> P (projecting twice is same as once)
- **Zero**: Orthogonal projectors annihilate each other
- **LhsSubspace**: LHS is a subspace of RHS, result is LHS
- **RhsSubspace**: RHS is a subspace of LHS, result is RHS

### 10.4 Addition Rules

When two projected expressions with the same argument are added:

| P_a(X) + P_b(X) | Result |
|------------------|--------|
| Vol(X) + Dev(X) | Sym(X) |
| Sym(X) + Skew(X) | X (identity) |

### 10.5 Construction-Time Rules

Applied when `dev()`, `sym()`, `vol()`, `skew()` are called:

```
dev(dev(A))     -> dev(A)       // idempotent
sym(sym(A))     -> sym(A)       // idempotent
vol(dev(A))     -> 0            // orthogonal
sym(skew(A))    -> 0            // orthogonal
dev(vol(A))     -> 0            // orthogonal
vol(sym(A))     -> vol(A)       // subspace
dev(sym(A))     -> dev(A)       // subspace
```

### 10.6 Projector Simplifier

The `tensor_projector_simplifier` visitor applies these rules post-construction:

1. **Inner product handler**: Detects `P : (P' : X)` patterns and applies contraction rules
2. **Addition handler**: Groups projector contractions by argument hash, applies addition rules
3. **Standalone contraction**: Detects `P : P'` (no argument) and simplifies

---

## 11. Hash System & Canonical Ordering

### 11.1 Hash Computation

Every expression node has a hash value computed from its structure:

```cpp
// For unary_op (sin, cos, neg, etc.):
hash = hash_combine(0, type_id)
hash = hash_combine(hash, child.hash)

// For binary_op (pow, scalar_mul, etc.):
hash = hash_combine(0, type_id)
hash = hash_combine(hash, lhs.hash)
hash = hash_combine(hash, rhs.hash)

// For n_ary_tree (add, mul):
hash = hash_combine(0, type_id)
sort(child_hashes)                    // commutativity!
for each sorted_hash:
    hash = hash_combine(hash, sorted_hash)
// NOTE: coefficient is NOT included in hash

// For symbols:
hash = hash_combine(0, name_hash)
```

### 11.2 Coefficient Exclusion Rule

The hash of `n_ary_tree` and certain `binary_op` nodes deliberately excludes numeric coefficients:

```
hash(3*x) == hash(x)           // scalar_mul coefficient excluded
hash(x^2) == hash(x)           // if exponent is constant
hash(5 + x + y) == hash(x + y) // add coefficient excluded
```

This enables the simplifier to:
1. Look up `x` in a hash-map of existing terms
2. Find `3*x` at the same hash slot
3. Merge: `3*x + 5*x -> 8*x`

Without this rule, `3*x` and `5*x` would hash differently and couldn't be merged in O(1).

### 11.3 Canonical Ordering

Print output uses `operator<` which compares hash values:

```
a < b  ⟺  hash(a) < hash(b)  OR  (hash(a) == hash(b) AND structural_less(a, b))
```

This means:
- Terms in a sum are printed in hash order (deterministic but not alphabetical)
- Factors in a product are printed in hash order
- Hash order is stable across runs (same input -> same output)

### 11.4 Hash Combine Function

```cpp
template <typename T>
void hash_combine(std::size_t& seed, const T& value) {
    seed ^= static_cast<std::size_t>(value) +
            static_cast<std::size_t>(0x9e3779b9) +
            (seed << 6) + (seed >> 2);
}
```

This is the Boost hash_combine algorithm using the golden ratio constant `0x9e3779b9 ≈ 2^32 / φ`. It provides good distribution with low collision rates.

---

## 12. Cross-Domain Interactions

### 12.1 Scalar ↔ Tensor

**Scalar-tensor multiplication:**
```cpp
auto expr = x * A;  // creates tensor_scalar_mul(x, A)
```
The `tensor_scalar_mul` node has `scalar_expression` LHS and `tensor_expression` RHS.

**Hash rule**: If the scalar is a constant, the hash equals the tensor's hash. This enables `3*A + 5*A -> 8*A`.

### 12.2 Scalar ↔ Tensor-to-Scalar

**Wrapping:**
When a scalar appears in a T2S context, it's wrapped in `tensor_to_scalar_scalar_wrapper`:
```cpp
auto f = x + trace(A);
// Internally: t2s_add(t2s_scalar_wrapper(x), tensor_trace(A))
```

**Unwrapping in simplifiers:**
When two scalar_wrappers are added, the simplifier unwraps them, performs scalar addition, and re-wraps:
```cpp
scalar_wrapper(x) + scalar_wrapper(y)
// Unwrap: x, y
// Scalar add: x + y
// Re-wrap: scalar_wrapper(x + y)
```

### 12.3 Tensor ↔ Tensor-to-Scalar

**T2S functions on tensors:**
```cpp
auto tr = trace(A);  // tensor -> T2S
auto d = det(A);     // tensor -> T2S
```

**T2S * Tensor:**
```cpp
auto expr = trace(A) * B;  // creates tensor_to_scalar_with_tensor_mul
```

**Differentiation bridge:**
```cpp
auto dA = diff(trace(A), A);  // T2S differentiated w.r.t. tensor -> tensor result
```

### 12.4 Type Promotion

The `promote_expr_fn` CPO handles type promotion when different domains interact:
```cpp
// scalar + T2S -> both become T2S (scalar is wrapped)
// T2S * tensor -> result is tensor (via tensor_to_scalar_with_tensor_mul)
```

---

## 13. Build System & Dependencies

### 13.1 CMake Configuration

```
numsim-cas/
├── CMakeLists.txt           # Main build file (207 LOC)
├── include/
│   └── numsim_cas/          # Public headers (192 files)
├── src/
│   └── numsim_cas/          # Implementation files (38 files)
├── tests/
│   ├── CMakeLists.txt       # Test build (53 LOC)
│   └── *.h / main.cpp       # Test sources (22 files)
├── examples/
│   └── CMakeLists.txt       # Example build
└── benchmarks/
    └── poly_verse_variant/  # Benchmark (WIP)
```

### 13.2 Building

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Test
ctest --test-dir build --output-on-failure

# With sanitizers
cmake -B build -DNUMSIM_CAS_SANITIZERS=ON
```

### 13.3 CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `NUMSIM_CAS_BUILD_TESTS` | `ON` | Build test suite |
| `NUMSIM_CAS_BUILD_EXAMPLES` | `OFF` | Build examples |
| `NUMSIM_CAS_BUILD_BENCHMARK` | `OFF` | Build benchmarks |
| `NUMSIM_CAS_SANITIZERS` | `OFF` | Enable ASAN + UBSAN |
| `NUMSIM_CAS_INSTALL_LIBRARY` | auto | Install targets |

### 13.4 Dependencies

| Dependency | Version | Source | Purpose |
|-----------|---------|--------|---------|
| tmech | master | FetchContent (GitHub) | Tensor numerics backend |
| GoogleTest | 1.15.2 | FetchContent (GitHub) | Testing framework |

### 13.5 Compiler Requirements

- **C++23** standard required (`cxx_std_23`)
- **GCC 14+**: Full support
- **Clang 18+**: Full support
- **MSVC (latest)**: Full support (with `/bigobj` for tests)

### 13.6 CI/CD

GitHub Actions workflow runs on every push and PR:
- Ubuntu 24.04: GCC-14 (Debug/Release), Clang-18 (Debug/Release)
- macOS latest: Apple Clang (Debug/Release)
- Windows latest: MSVC (Debug/Release)
- Separate clang-format check workflow

---

## 14. File Reference

### 14.1 Core Layer (`include/numsim_cas/core/`)

| File | Purpose |
|------|---------|
| `expression.h` | Abstract base class for all nodes |
| `expression_holder.h` | RAII wrapper (shared_ptr) |
| `visitor_base.h` | Virtual visitor infrastructure (3 variants) |
| `tag_invoke.h` | CPO foundation |
| `binary_ops.h` | Binary operator CPO tags (add, sub, mul, div) |
| `operators.h` | Global operator overloads |
| `n_ary_tree.h` | Hash-map based commutative container |
| `n_ary_vector.h` | Vector-based ordered container |
| `binary_op.h` | Binary node base template |
| `unary_op.h` | Unary node base template |
| `symbol_base.h` | Symbol/variable base template |
| `domain_traits.h` | Domain trait template + helpers |
| `hash_functions.h` | Hash combining primitives |
| `scalar_number.h` | Type-erasing numeric wrapper |
| `assumptions.h` | Expression assumptions and relations |
| `evaluator_base.h` | Evaluation infrastructure |
| `diff.h` | Differentiation CPO |
| `substitute.h` | Substitution CPO |
| `promote_expr.h` | Type promotion CPO |
| `make_constant.h` | Constant creation CPO |
| `make_negative.h` | Negation CPO |
| `contains_expression.h` | Subexpression search utilities |
| `limit_result.h` | Limit behavior types |
| `cas_error.h` | Error types |
| `core_fwd.h` | Forward declarations |
| `print_mul_fractions.h` | Fraction printing helper |

### 14.2 Core Simplifiers (`include/numsim_cas/core/simplifier/`)

| File | Purpose |
|------|---------|
| `simplifier_add.h` | Generic addition simplification (~510 LOC) |
| `simplifier_sub.h` | Generic subtraction simplification (~419 LOC) |
| `simplifier_mul.h` | Generic multiplication simplification (~88 LOC) |
| `simplifier_pow.h` | Generic power simplification (~179 LOC) |

### 14.3 Scalar Domain (`include/numsim_cas/scalar/`)

| File | Purpose |
|------|---------|
| `scalar_expression.h` | Scalar expression base class |
| `scalar_node_list.h` | Node type registry macro |
| `scalar_visitor_typedef.h` | Visitor type definitions |
| `scalar_domain_traits.h` | Scalar domain trait specialization |
| `scalar_operators.h` | Operator CPO implementations |
| `scalar_std.h` | Math functions (sin, cos, pow, etc.) |
| `scalar_functions.h` | Utility functions |
| `scalar_globals.h` | Global zero/one accessors |
| `scalar_make_constant.h` | Constant creation |
| `scalar_assume.h` | Assumption helpers |
| `scalar_diff.h` | Differentiation entry point |
| `scalar_io.h` | Stream output |
| `scalar.h` | Symbol node |
| `scalar_zero.h`, `scalar_one.h`, `scalar_constant.h` | Constant nodes |
| `scalar_add.h`, `scalar_mul.h` | N-ary arithmetic |
| `scalar_negative.h`, `scalar_power.h` | Unary/binary arithmetic |
| `scalar_sin.h` ... `scalar_sign.h` | Function nodes |
| `scalar_div.h`, `scalar_rational.h` | Division nodes |
| `scalar_named_expression.h` | Named function wrapper |

### 14.4 Scalar Simplifiers (`include/numsim_cas/scalar/simplifier/`)

| File | Purpose |
|------|---------|
| `scalar_simplifier_add.h` | Addition dispatcher + visitors |
| `scalar_simplifier_sub.h` | Subtraction dispatcher + visitors |
| `scalar_simplifier_mul.h` | Multiplication dispatcher + visitors |
| `scalar_simplifier_pow.h` | Power dispatcher + visitors |

### 14.5 Scalar Visitors (`include/numsim_cas/scalar/visitors/`)

| File | Purpose |
|------|---------|
| `scalar_evaluator.h` | Numeric evaluation |
| `scalar_printer.h` | String output with precedence |
| `scalar_differentiation.h` | Symbolic differentiation |
| `scalar_rebuild_visitor.h` | Expression transformation base |
| `scalar_substitution.h` | Find-and-replace |

### 14.6 Tensor Domain (`include/numsim_cas/tensor/`)

| File | Purpose |
|------|---------|
| `tensor_expression.h` | Tensor expression base class |
| `tensor_node_list.h` | Node type registry |
| `tensor_visitor_typedef.h` | Visitor type definitions |
| `tensor_domain_traits.h` | Tensor domain traits |
| `tensor_operators.h` | Operator CPO implementations |
| `tensor_functions.h` | High-level API (sym, dev, inner_product, etc.) |
| `tensor_space.h` | Algebraic space classification |
| `sequence.h` | Index sequence manipulation |
| `tensor_assume.h` | Tensor assumptions |
| `tensor_io.h` | Stream output |
| `projection_tensor.h` | Projector node and factories |
| `projector_algebra.h` | Projector algebraic rules |
| `tensor.h` | Symbol node |
| `tensor_zero.h` | Zero tensor |
| `identity_tensor.h`, `kronecker_delta.h` | Identity/delta nodes |
| `tensor_add.h`, `tensor_mul.h`, `tensor_pow.h` | Arithmetic nodes |
| `tensor_negative.h`, `tensor_scalar_mul.h` | Unary/binary arithmetic |
| `tensor_inv.h` | Matrix inverse |
| `inner_product_wrapper.h`, `outer_product_wrapper.h` | Product nodes |
| `basis_change_imp.h` | Index permutation |
| `simple_outer_product.h` | Iterated outer product |

### 14.7 Tensor Simplifiers (`include/numsim_cas/tensor/simplifier/`)

| File | Purpose |
|------|---------|
| `tensor_simplifier_add.h` | Tensor addition simplification |
| `tensor_simplifier_mul.h` | Tensor multiplication simplification |
| `tensor_with_scalar_simplifier_mul.h` | Scalar-tensor multiplication |
| `tensor_inner_product_simplifier.h` | Inner product simplification |
| `tensor_projector_simplifier.h` | Projector algebra simplification |

### 14.8 Tensor Visitors (`include/numsim_cas/tensor/visitors/`)

| File | Purpose |
|------|---------|
| `tensor_evaluator.h` | Numeric evaluation (tmech backend) |
| `tensor_printer.h` | String output |
| `tensor_differentiation.h` | Symbolic differentiation |
| `tensor_rebuild_visitor.h` | Expression transformation base |
| `tensor_substitution.h` | Find-and-replace |

### 14.9 Tensor-to-Scalar Domain (`include/numsim_cas/tensor_to_scalar/`)

| File | Purpose |
|------|---------|
| `tensor_to_scalar_expression.h` | T2S expression base class |
| `tensor_to_scalar_node_list.h` | Node type registry |
| `tensor_to_scalar_visitor_typedef.h` | Visitor type definitions |
| `tensor_to_scalar_domain_traits.h` | T2S domain traits |
| `tensor_to_scalar_operators.h` | Operator CPO implementations |
| `tensor_to_scalar_std.h` | pow() and log() functions |
| `tensor_to_scalar_functions.h` | trace(), det(), norm(), dot() |
| `tensor_to_scalar_io.h` | Stream output |
| `tensor_trace.h`, `tensor_det.h`, `tensor_norm.h`, `tensor_dot.h` | Unary nodes |
| `tensor_to_scalar_zero.h`, `tensor_to_scalar_one.h` | Constants |
| `tensor_to_scalar_scalar_wrapper.h` | Scalar bridge |
| `tensor_to_scalar_add.h`, `tensor_to_scalar_mul.h`, `tensor_to_scalar_pow.h` | Arithmetic |
| `tensor_to_scalar_negative.h`, `tensor_to_scalar_log.h` | Unary operations |
| `tensor_inner_product_to_scalar.h` | Indexed inner product |

### 14.10 Test Files (`tests/`)

| File | Domain | Description |
|------|--------|-------------|
| `ScalarExpressionTest.h` | Scalar | Arithmetic, printing, canonicalization |
| `ScalarDifferentiationTest.h` | Scalar | All derivative rules |
| `ScalarEvaluatorTest.h` | Scalar | Numeric evaluation |
| `ScalarAssumptionTest.h` | Scalar | Assumption inference |
| `ScalarSubstitutionTest.h` | Scalar | Expression substitution |
| `TensorExpressionTest.h` | Tensor | Arithmetic, products, projections |
| `TensorDifferentiationTest.h` | Tensor | Tensor derivatives |
| `TensorEvaluatorTest.h` | Tensor | Numeric evaluation with tmech |
| `TensorProjectorDifferentiationTest.h` | Tensor | Projector derivatives |
| `TensorSpacePropagationTest.h` | Tensor | Space tracking through operations |
| `TensorSubstitutionTest.h` | Tensor | Tensor substitution |
| `TensorToScalarExpressionTest.h` | T2S | T2S arithmetic |
| `TensorToScalarDifferentiationTest.h` | T2S | T2S derivatives |
| `TensorToScalarEvaluatorTest.h` | T2S | T2S evaluation |
| `TensorToScalarSubstitutionTest.h` | T2S | T2S substitution |
| `CoreBugFixTest.h` | Core | Regression tests |
| `LimitVisitorTest.h` | Core | Limit analysis |
| `cas_test_helpers.h` | All | EXPECT_PRINT, EXPECT_SAME_PRINT macros |
| `main.cpp` | All | Test runner entry point |

---

## 15. Glossary

| Term | Definition |
|------|-----------|
| **CAS** | Computer Algebra System -- software for symbolic mathematics |
| **CPO** | Customization Point Object -- a callable that dispatches to user-provided implementations via ADL |
| **CRTP** | Curiously Recurring Template Pattern -- a template pattern where a class inherits from a template parameterized by itself |
| **DAG** | Directed Acyclic Graph -- a graph with directed edges and no cycles |
| **Domain** | One of the three expression types: scalar, tensor, tensor-to-scalar |
| **Domain traits** | A traits template providing type and function information for a domain |
| **Expression holder** | RAII wrapper around shared_ptr to an expression node |
| **Hash combine** | Function that mixes a new value into an existing hash seed |
| **Inner product** | Tensor contraction over shared indices (e.g., A:B = A_ij * B_ij) |
| **N-ary tree** | A tree node with variable number of children (used for add/mul) |
| **Node** | A concrete expression type (e.g., scalar_add, tensor_pow) |
| **Node list macro** | X-macro listing all node types for visitor generation |
| **Outer product** | Tensor product without contraction (A ⊗ B)_ijkl = A_ij * B_kl |
| **Projection tensor** | Fourth-order tensor that extracts a specific component (sym, dev, vol, skew) |
| **RAII** | Resource Acquisition Is Initialization -- C++ idiom tying resource lifecycle to object scope |
| **Sequence** | Ordered list of indices for tensor operations (1-based input, 0-based storage) |
| **Tag invoke** | A pattern for ADL-based customization using tag types |
| **Tensor space** | Classification of tensor by symmetry and trace properties |
| **T2S** | Tensor-to-scalar -- expressions that map tensors to scalars (trace, det, norm, dot) |
| **tmech** | C++ tensor mechanics library used as numerical backend |
| **Visitable** | A node that can accept a visitor (has accept() methods) |
| **Visitor** | A class with operator() overloads for each node type |
