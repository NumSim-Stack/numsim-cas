// #ifndef COMPARE_EQUAL_VISITOR_H
// #define COMPARE_EQUAL_VISITOR_H

// #include "expression_holder.h"
// #include "scalar/scalar_type_defs.h" // scalar_visitor_const_t,
// scalar_visitable_t, node fwd decls

// namespace numsim::cas {

// class compare_equal_visitor {
// public:
//   using expr_holder_t = expression_holder<scalar_expression>;

//   bool operator()(expr_holder_t const& lhs, expr_holder_t const& rhs) const
//   noexcept {
//     if (!lhs.is_valid() || !rhs.is_valid())
//       return lhs.is_valid() == rhs.is_valid(); // both invalid => equal, else
//       false

//     bool result = false;

//     lhs_dispatch lhs_vis(rhs, result);
//     as_visitable(lhs.get()).accept(lhs_vis);
//     return result;
//   }

// private:
//   // helper: downcast scalar_expression -> scalar_visitable_t
//   // (works because actual object is visitable_impl<scalar_expression,...>)
//   static scalar_visitable_t const& as_visitable(scalar_expression const& e)
//   noexcept {
//     return static_cast<scalar_visitable_t const&>(e);
//   }

//          // -------- RHS visitor: compares rhs against a concrete LHS node
//          --------
//   template <typename LHSNode>
//   class rhs_dispatch final : public scalar_visitor_const_t {
//   public:
//     rhs_dispatch(LHSNode const& lhs, bool& out) noexcept : lhs_(lhs),
//     out_(out) {}

//            // implement all node overloads via one handler
//     void operator()(scalar const& rhs)            noexcept override {
//     handle(rhs); } void operator()(scalar_zero const& rhs)       noexcept
//     override { handle(rhs); } void operator()(scalar_one const& rhs) noexcept
//     override { handle(rhs); } void operator()(scalar_constant const& rhs)
//     noexcept override { handle(rhs); }

//     void operator()(scalar_div const& rhs)        noexcept override {
//     handle(rhs); } void operator()(scalar_add const& rhs)        noexcept
//     override { handle(rhs); } void operator()(scalar_mul const& rhs) noexcept
//     override { handle(rhs); } void operator()(scalar_negative const& rhs)
//     noexcept override { handle(rhs); } void operator()(scalar_named_expression const&
//     rhs)   noexcept override { handle(rhs); }

//     void operator()(scalar_sin const& rhs)        noexcept override {
//     handle(rhs); } void operator()(scalar_cos const& rhs)        noexcept
//     override { handle(rhs); } void operator()(scalar_tan const& rhs) noexcept
//     override { handle(rhs); } void operator()(scalar_asin const& rhs)
//     noexcept override { handle(rhs); } void operator()(scalar_acos const&
//     rhs)       noexcept override { handle(rhs); } void operator()(scalar_atan
//     const& rhs)       noexcept override { handle(rhs); }

//     void operator()(scalar_pow const& rhs)        noexcept override {
//     handle(rhs); } void operator()(scalar_sqrt const& rhs)       noexcept
//     override { handle(rhs); } void operator()(scalar_log const& rhs) noexcept
//     override { handle(rhs); } void operator()(scalar_exp const& rhs) noexcept
//     override { handle(rhs); } void operator()(scalar_sign const& rhs)
//     noexcept override { handle(rhs); } void operator()(scalar_abs const& rhs)
//     noexcept override { handle(rhs); }

//   private:
//     template <typename RHSNode>
//     void handle(RHSNode const& rhs) noexcept {
//       if constexpr (std::is_same_v<LHSNode, RHSNode>) {
//         out_ = (lhs_ == rhs);
//       } else {
//         out_ = false;
//       }
//     }

//     LHSNode const& lhs_;
//     bool&          out_;
//   };

//          // -------- LHS visitor: picks LHS dynamic type, then visits RHS
//          --------
//   class lhs_dispatch final : public scalar_visitor_const_t {
//   public:
//     lhs_dispatch(expr_holder_t const& rhs, bool& out) noexcept : rhs_(rhs),
//     out_(out) {}

//     void operator()(scalar const& lhs)            noexcept override {
//     dispatch(lhs); } void operator()(scalar_zero const& lhs)       noexcept
//     override { dispatch(lhs); } void operator()(scalar_one const& lhs)
//     noexcept override { dispatch(lhs); } void operator()(scalar_constant
//     const& lhs)   noexcept override { dispatch(lhs); }

//     void operator()(scalar_div const& lhs)        noexcept override {
//     dispatch(lhs); } void operator()(scalar_add const& lhs)        noexcept
//     override { dispatch(lhs); } void operator()(scalar_mul const& lhs)
//     noexcept override { dispatch(lhs); } void operator()(scalar_negative
//     const& lhs)   noexcept override { dispatch(lhs); } void
//     operator()(scalar_named_expression const& lhs)   noexcept override {
//     dispatch(lhs); }

//     void operator()(scalar_sin const& lhs)        noexcept override {
//     dispatch(lhs); } void operator()(scalar_cos const& lhs)        noexcept
//     override { dispatch(lhs); } void operator()(scalar_tan const& lhs)
//     noexcept override { dispatch(lhs); } void operator()(scalar_asin const&
//     lhs)       noexcept override { dispatch(lhs); } void
//     operator()(scalar_acos const& lhs)       noexcept override {
//     dispatch(lhs); } void operator()(scalar_atan const& lhs)       noexcept
//     override { dispatch(lhs); }

//     void operator()(scalar_pow const& lhs)        noexcept override {
//     dispatch(lhs); } void operator()(scalar_sqrt const& lhs)       noexcept
//     override { dispatch(lhs); } void operator()(scalar_log const& lhs)
//     noexcept override { dispatch(lhs); } void operator()(scalar_exp const&
//     lhs)        noexcept override { dispatch(lhs); } void
//     operator()(scalar_sign const& lhs)       noexcept override {
//     dispatch(lhs); } void operator()(scalar_abs const& lhs)        noexcept
//     override { dispatch(lhs); }

//   private:
//     template <typename LHSNode>
//     void dispatch(LHSNode const& lhs) noexcept {
//       rhs_dispatch<LHSNode> rhs_vis(lhs, out_);
//       as_visitable(rhs_.get()).accept(rhs_vis);
//     }

//     expr_holder_t const& rhs_;
//     bool&                out_;

//     static scalar_visitable_t const& as_visitable(scalar_expression const& e)
//     noexcept {
//       return static_cast<scalar_visitable_t const&>(e);
//     }
//   };
// };

// } // namespace numsim::cas

// #endif // COMPARE_EQUAL_VISITOR_H
