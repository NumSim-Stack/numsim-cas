#ifndef SCALAR_LATEX_PRINTER_H
#define SCALAR_LATEX_PRINTER_H

#include <numsim_cas/latex_printer_base.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_functions.h>

namespace numsim::cas {

template <typename StreamType>
class scalar_latex_printer final
    : public scalar_visitor_const_t,
      public latex_printer_base<scalar_latex_printer<StreamType>, StreamType> {
public:
  using base = latex_printer_base<scalar_latex_printer<StreamType>, StreamType>;
  using expr_t = expression_holder<scalar_expression>;
  using base_visitor = scalar_visitor_const_t;
  using base::begin;
  using base::end;
  using base::print_unary;

  explicit scalar_latex_printer(
      StreamType &out, latex_config const &cfg = latex_config::default_config())
      : base(out, cfg) {}

  scalar_latex_printer(scalar_latex_printer const &) = delete;
  scalar_latex_printer(scalar_latex_printer &&) = delete;
  const scalar_latex_printer &operator=(scalar_latex_printer const &) = delete;

  void apply(expression_holder<scalar_expression> const &expr,
             Precedence parent_precedence = Precedence::None);

  void operator()(scalar const &visitable) override;
  void operator()(scalar_named_expression const &visitable) override;
  void operator()(scalar_constant const &visitable) override;
  void operator()(scalar_one const &visitable) override;
  void operator()(scalar_zero const &visitable) override;
  void operator()(scalar_mul const &visitable) override;
  void operator()(scalar_add const &visitable) override;
  void operator()(scalar_negative const &visitable) override;
  void operator()(scalar_log const &visitable) override;
  void operator()(scalar_sqrt const &visitable) override;
  void operator()(scalar_exp const &visitable) override;
  void operator()(scalar_sign const &visitable) override;
  void operator()(scalar_abs const &visitable) override;
  void operator()(scalar_pow const &visitable) override;
  void operator()(scalar_tan const &visitable) override;
  void operator()(scalar_sin const &visitable) override;
  void operator()(scalar_cos const &visitable) override;
  void operator()(scalar_atan const &visitable) override;
  void operator()(scalar_asin const &visitable) override;
  void operator()(scalar_acos const &visitable) override;

  template <class T> void operator()([[maybe_unused]] T const &visitable) {
    static_assert(sizeof(T) == 0,
                  "scalar_latex_printer: missing overload for this node type");
  }

private:
  Precedence m_parent_precedence{Precedence::None};
  using base::m_config;
  using base::m_first_term;
  using base::m_out;
};

} // namespace numsim::cas

#endif // SCALAR_LATEX_PRINTER_H
