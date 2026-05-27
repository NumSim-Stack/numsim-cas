#include <numsim_cas/parser/symbol_table.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/scalar/scalar.h>
#include <numsim_cas/tensor/tensor.h>

#include <sstream>

namespace numsim::cas::parser {

expression_holder<scalar_expression>
symbol_table::get_or_declare_scalar(std::string_view name) {
  std::string key(name);
  auto it = m_entries.find(key);
  if (it == m_entries.end()) {
    auto expr = make_expression<scalar>(key);
    m_entries.emplace(std::move(key), scalar_entry{expr});
    return expr;
  }
  if (auto *s = std::get_if<scalar_entry>(&it->second)) {
    return s->expr;
  }
  // Existing entry is a tensor — cross-type collision.
  throw type_collision_error(
      "'" + key + "' is already declared as a tensor; cannot reuse as a scalar",
      0, std::string_view{});
}

expression_holder<tensor_expression>
symbol_table::get_or_declare_tensor(std::string_view name, std::size_t rank,
                                    std::size_t dim) {
  std::string key(name);
  auto it = m_entries.find(key);
  if (it == m_entries.end()) {
    auto expr = make_expression<tensor>(key, dim, rank);
    m_entries.emplace(std::move(key), tensor_entry{expr, rank, dim});
    return expr;
  }
  if (auto *t = std::get_if<tensor_entry>(&it->second)) {
    if (t->rank != rank || t->dim != dim) {
      std::ostringstream oss;
      oss << "'" << key
          << "' is already declared as a tensor with rank=" << t->rank
          << ", dim=" << t->dim << "; cannot redeclare with rank=" << rank
          << ", dim=" << dim;
      throw redeclaration_error(oss.str(), 0, std::string_view{});
    }
    return t->expr;
  }
  // Existing entry is a scalar — cross-type collision.
  throw type_collision_error(
      "'" + key + "' is already declared as a scalar; cannot reuse as a tensor",
      0, std::string_view{});
}

bool symbol_table::has(std::string_view name) const noexcept {
  // unordered_map::find takes string; we materialise a key.
  // C++20 heterogeneous lookup would avoid the allocation but requires
  // a custom hash/equal — not worth the complexity for occasional lookup.
  return m_entries.find(std::string(name)) != m_entries.end();
}

std::optional<std::pair<std::size_t, std::size_t>>
symbol_table::tensor_shape(std::string_view name) const {
  auto it = m_entries.find(std::string(name));
  if (it == m_entries.end())
    return std::nullopt;
  if (auto const *t = std::get_if<tensor_entry>(&it->second)) {
    return std::make_pair(t->rank, t->dim);
  }
  return std::nullopt;
}

} // namespace numsim::cas::parser
