#ifndef PREDICATE_LATEX_PRINTER_H
#define PREDICATE_LATEX_PRINTER_H

#include <numsim_cas/predicate/predicate_definitions.h>

namespace numsim::cas {

// LaTeX printer for the predicate domain. Mirrors predicate_printer but
// emits LaTeX glyphs — `\top` / `\bot` for the two literals.
template <typename StreamType>
class predicate_latex_printer final : public predicate_visitor_const_t {
public:
  explicit predicate_latex_printer(StreamType &out) : m_out(out) {}

  predicate_latex_printer(predicate_latex_printer const &) = delete;
  predicate_latex_printer(predicate_latex_printer &&) = delete;
  predicate_latex_printer &operator=(predicate_latex_printer const &) = delete;

  void apply(expression_holder<predicate_expression> const &expr) {
    if (!expr.is_valid()) {
      return;
    }
    static_cast<predicate_visitable_t const &>(expr.get())
        .accept(static_cast<predicate_visitor_const_t &>(*this));
  }

  void operator()([[maybe_unused]] predicate_true const &) override {
    m_out << "\\top";
  }

  void operator()([[maybe_unused]] predicate_false const &) override {
    m_out << "\\bot";
  }

private:
  StreamType &m_out;
};

} // namespace numsim::cas

#endif // PREDICATE_LATEX_PRINTER_H
