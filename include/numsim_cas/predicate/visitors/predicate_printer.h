#ifndef PREDICATE_PRINTER_H
#define PREDICATE_PRINTER_H

#include <numsim_cas/predicate/predicate_definitions.h>

namespace numsim::cas {

// Header-only printer for the predicate domain. Writes `true` / `false`
// (and, in subsequent PRs, the compound forms `a < b`, `a && b`, `!a`, etc.)
// to the supplied stream.
//
// Symmetric with the existing scalar_printer / tensor_printer structure
// but without the precedence machinery — the only inhabitants of the
// domain so far are the two literal leaves, which never need parens.
// Precedence handling will be folded in alongside the comparison and
// logical combinator nodes (#136).
template <typename StreamType>
class predicate_printer final : public predicate_visitor_const_t {
public:
  explicit predicate_printer(StreamType &out) : m_out(out) {}

  predicate_printer(predicate_printer const &) = delete;
  predicate_printer(predicate_printer &&) = delete;
  predicate_printer &operator=(predicate_printer const &) = delete;

  void apply(expression_holder<predicate_expression> const &expr) {
    if (!expr.is_valid()) {
      return;
    }
    static_cast<predicate_visitable_t const &>(expr.get())
        .accept(static_cast<predicate_visitor_const_t &>(*this));
  }

  void operator()([[maybe_unused]] predicate_true const &) override {
    m_out << "true";
  }

  void operator()([[maybe_unused]] predicate_false const &) override {
    m_out << "false";
  }

private:
  StreamType &m_out;
};

} // namespace numsim::cas

#endif // PREDICATE_PRINTER_H
