// #ifndef SCALAR_DIV_H
// #define SCALAR_DIV_H

// #include <numsim_cas/core/binary_op.h>
// #include <numsim_cas/scalar/scalar_expression.h>

// namespace numsim::cas {

// class scalar_div final : public binary_op<scalar_node_base_t<scalar_div>> {
// public:
//   using base = binary_op<scalar_node_base_t<scalar_div>>;

//   using base::base;
//   scalar_div() = delete;
//   scalar_div(scalar_div &&data) : base(std::move(static_cast<base &&>(data)))
//   {} scalar_div(scalar_div const &data) : base(static_cast<base const
//   &>(data)) {} ~scalar_div() = default; const scalar_div
//   &operator=(scalar_div &&) = delete;
// };

// } // namespace numsim::cas

// #endif // SCALAR_DIV_H
