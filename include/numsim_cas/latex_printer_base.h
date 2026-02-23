#ifndef LATEX_PRINTER_BASE_H
#define LATEX_PRINTER_BASE_H

#include <numsim_cas/latex_config.h>
#include <numsim_cas/printer_base.h>
#include <string_view>

namespace numsim::cas {

template <typename Derived, typename StreamType>
class latex_printer_base : public printer_base<Derived, StreamType> {
public:
  using base = printer_base<Derived, StreamType>;
  friend Derived;

  latex_printer_base(StreamType &out, latex_config const &cfg)
      : base(out), m_config(cfg) {}

protected:
  void begin(Precedence current_precedence,
             Precedence parent_precedence) noexcept {
    if (current_precedence < parent_precedence)
      this->m_out << "\\left(";
  }

  void end(Precedence current_precedence,
           Precedence parent_precedence) noexcept {
    if (current_precedence < parent_precedence)
      this->m_out << "\\right)";
  }

  template <typename Visitable>
  void print_unary(std::string_view name, Visitable const &visitable) noexcept {
    this->m_out << name << "\\left(";
    static_cast<Derived &>(*this).apply(visitable.expr());
    this->m_out << "\\right)";
  }

  latex_config const &m_config;
};

} // namespace numsim::cas

#endif // LATEX_PRINTER_BASE_H
