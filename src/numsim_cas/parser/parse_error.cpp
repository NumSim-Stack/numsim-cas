#include <numsim_cas/parser/parse_error.h>

#include <algorithm>
#include <sstream>
#include <utility>

namespace numsim::cas::parser {

namespace {

struct line_info {
  std::size_t line;
  std::size_t column;
  std::size_t line_start;
  std::size_t line_end;
};

// Walk the source to find the line, column, and bounding newlines for
// the byte offset. O(N) in the source length; runs at most twice per
// parse_error construction (once in format_message, once in the body).
// Parse errors are rare so the duplication is negligible.
line_info resolve_line(std::string_view source, std::size_t pos) noexcept {
  pos = std::min(pos, source.size());
  line_info info{1, 1, 0, source.size()};
  for (std::size_t i = 0; i < pos; ++i) {
    if (source[i] == '\n') {
      ++info.line;
      info.line_start = i + 1;
    }
  }
  for (std::size_t i = pos; i < source.size(); ++i) {
    if (source[i] == '\n') {
      info.line_end = i;
      break;
    }
  }
  info.column = pos - info.line_start + 1;
  return info;
}

// Build the formatted what() string. Two formats depending on whether
// source context was supplied:
//   - non-empty source: full snippet with line:col header and caret
//   - empty source:     just "parse error: <body>"
std::string format_message(std::string_view body, std::string_view source,
                           std::size_t byte_offset) {
  if (source.empty()) {
    std::string out = "parse error: ";
    out.append(body);
    return out;
  }
  auto info = resolve_line(source, byte_offset);
  std::ostringstream oss;
  oss << "parse error at " << info.line << ":" << info.column << ": " << body;
  if (info.line_start <= info.line_end) {
    auto line_text =
        source.substr(info.line_start, info.line_end - info.line_start);
    // Window long lines around the caret so what() stays readable; the
    // ellipsis is part of the snippet, so the caret shifts with it.
    constexpr std::size_t context = 60;
    std::size_t caret = info.column - 1;
    if (line_text.size() > 2 * context) {
      std::size_t begin = caret > context ? caret - context : 0;
      std::size_t end = std::min(line_text.size(), caret + context);
      std::string_view window = line_text.substr(begin, end - begin);
      std::string prefix = begin > 0 ? "..." : "";
      std::string suffix = end < line_text.size() ? "..." : "";
      oss << "\n  " << prefix << window << suffix;
      caret = caret - begin + prefix.size();
    } else {
      oss << "\n  " << line_text;
    }
    oss << "\n  " << std::string(caret, ' ') << '^';
  }
  return oss.str();
}

} // namespace

parse_error::parse_error(std::string const &message, std::size_t byte_offset,
                         std::string_view source)
    : cas_error(format_message(message, source, byte_offset)) {
  if (!source.empty()) {
    auto info = resolve_line(source, byte_offset);
    m_pos = std::min(byte_offset, source.size());
    m_line = info.line;
    m_col = info.column;
    m_has_position = true;
  }
}

unknown_symbol_error::unknown_symbol_error(std::string name,
                                           std::size_t byte_offset,
                                           std::string_view source)
    : parse_error("unknown identifier: '" + name + "'", byte_offset, source),
      m_name(std::move(name)) {}

unknown_function_error::unknown_function_error(std::string name,
                                               std::size_t byte_offset,
                                               std::string_view source)
    : parse_error("unknown function: '" + name + "'", byte_offset, source),
      m_name(std::move(name)) {}

namespace {
// "expects 1 argument" / "expects 1 or 4 arguments" / "expects 1, 2 or 4
// arguments" — every registered arity, so an overloaded name does not report
// whichever overload happened to be visited first.
std::string format_arity(std::string_view function,
                         std::vector<std::size_t> const &expected,
                         std::size_t actual) {
  std::ostringstream oss;
  oss << "function '" << function << "' expects ";
  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (i != 0) {
      oss << (i + 1 == expected.size() ? " or " : ", ");
    }
    oss << expected[i];
  }
  const bool plural = expected.size() != 1 || expected.front() != 1;
  oss << " argument" << (plural ? "s" : "") << ", got " << actual;
  return oss.str();
}

std::vector<std::size_t> normalize(std::vector<std::size_t> arities) {
  std::sort(arities.begin(), arities.end());
  arities.erase(std::unique(arities.begin(), arities.end()), arities.end());
  if (arities.empty()) {
    arities.push_back(0);
  }
  return arities;
}
} // namespace

arity_error::arity_error(std::string function_name, std::size_t expected_arity_,
                         std::size_t actual_arity_, std::size_t byte_offset,
                         std::string_view source)
    : arity_error(std::move(function_name),
                  std::vector<std::size_t>{expected_arity_}, actual_arity_,
                  byte_offset, source) {}

arity_error::arity_error(std::string function_name,
                         std::vector<std::size_t> expected_arities_,
                         std::size_t actual_arity_, std::size_t byte_offset,
                         std::string_view source)
    : arity_error(std::move(function_name), normalize(expected_arities_),
                  actual_arity_, byte_offset, source, normalized_tag{}) {}

arity_error::arity_error(std::string function_name,
                         std::vector<std::size_t> expected_arities_,
                         std::size_t actual_arity_, std::size_t byte_offset,
                         std::string_view source, normalized_tag)
    : parse_error(format_arity(function_name, expected_arities_, actual_arity_),
                  byte_offset, source),
      m_function(std::move(function_name)),
      m_expected(std::move(expected_arities_)), m_actual(actual_arity_) {}

} // namespace numsim::cas::parser
