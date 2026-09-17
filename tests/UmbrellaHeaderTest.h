#ifndef UMBRELLAHEADERTEST_H
#define UMBRELLAHEADERTEST_H

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#ifdef NUMSIM_CAS_INCLUDE_DIR

namespace umbrella_detail {

namespace fs = std::filesystem;

// Headers deliberately not exported from numsim_cas.h. Anything else that
// stops being reachable is a regression: a consumer following the documented
// include pattern would have to discover an internal path to use it.
inline std::set<std::string> excluded_headers() {
  return {
      // Optional component: only built with NUMSIM_CAS_BUILD_PARSER, and
      // consumers include <numsim_cas/parser/parser.h> explicitly.
      "parser/parse_error.h",
      "parser/parser.h",
      "parser/symbol_table.h",
      // Implementation of to_latex(), which the *_latex_io.h facades declare.
      "latex_printer_base.h",
      "scalar/visitors/scalar_latex_printer.h",
      "tensor/visitors/tensor_latex_printer.h",
      "tensor_to_scalar/visitors/tensor_to_scalar_latex_printer.h",
      // Internal visitors with no user-facing entry point.
      "scalar/visitors/scalar_assumption_propagator.h",
      "scalar/visitors/scalar_comparison_print_helper.h",
      // Wired to nothing; kept until it is either used or removed.
      "tensor/simplifier/tensor_projector_simplifier.h",
  };
}

inline fs::path include_root() { return fs::path{NUMSIM_CAS_INCLUDE_DIR}; }

// Relative paths of every header shipped under include/numsim_cas/.
inline std::set<std::string> all_headers() {
  std::set<std::string> headers;
  const auto root = include_root() / "numsim_cas";
  for (auto const &entry : fs::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".h")
      continue;
    headers.insert(fs::relative(entry.path(), root).generic_string());
  }
  return headers;
}

// Headers included by one header, as paths relative to include/numsim_cas/.
// Both spellings occur: <numsim_cas/x.h> and "x.h" relative to the includer.
inline std::vector<std::string>
direct_includes(fs::path const &header, std::string const &relative_to) {
  std::vector<std::string> targets;
  std::ifstream in(header);
  std::string line;
  const std::string angle = "#include <numsim_cas/";
  const std::string quote = "#include \"";
  const auto dir = fs::path{relative_to}.parent_path();
  while (std::getline(in, line)) {
    if (const auto start = line.find(angle); start != std::string::npos) {
      const auto first = start + angle.size();
      if (const auto last = line.find('>', first); last != std::string::npos)
        targets.push_back(line.substr(first, last - first));
      continue;
    }
    if (const auto start = line.find(quote); start != std::string::npos) {
      const auto first = start + quote.size();
      const auto last = line.find('"', first);
      if (last == std::string::npos)
        continue;
      const auto target = line.substr(first, last - first);
      targets.push_back((dir / target).lexically_normal().generic_string());
    }
  }
  return targets;
}

// Headers reachable from numsim_cas.h by following includes.
inline std::set<std::string> reachable_headers() {
  const auto root = include_root() / "numsim_cas";
  std::set<std::string> seen;
  std::vector<std::string> pending{"numsim_cas.h"};
  while (!pending.empty()) {
    const auto current = pending.back();
    pending.pop_back();
    if (!seen.insert(current).second)
      continue;
    const auto path = root / current;
    if (!fs::exists(path))
      continue;
    for (auto const &target : direct_includes(path, current))
      pending.push_back(target);
  }
  return seen;
}

} // namespace umbrella_detail

// Every shipped header is reachable from the umbrella, or listed as excluded.
TEST(UmbrellaHeader, ExportsEveryPublicHeader) {
  const auto reachable = umbrella_detail::reachable_headers();
  const auto excluded = umbrella_detail::excluded_headers();
  std::vector<std::string> unreachable;
  for (auto const &header : umbrella_detail::all_headers()) {
    if (!reachable.count(header) && !excluded.count(header))
      unreachable.push_back(header);
  }
  EXPECT_TRUE(unreachable.empty())
      << "not reachable from numsim_cas.h:\n  " << [&] {
           std::string joined;
           for (auto const &header : unreachable)
             joined += header + "\n  ";
           return joined;
         }();
}

// The exclusion list names real headers, so a renamed or deleted header
// cannot leave a stale entry masking a genuine gap.
TEST(UmbrellaHeader, ExclusionsExist) {
  const auto headers = umbrella_detail::all_headers();
  for (auto const &excluded : umbrella_detail::excluded_headers())
    EXPECT_TRUE(headers.count(excluded)) << "stale exclusion: " << excluded;
}

#endif // NUMSIM_CAS_INCLUDE_DIR

#endif // UMBRELLAHEADERTEST_H
