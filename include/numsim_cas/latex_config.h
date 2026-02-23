#ifndef LATEX_CONFIG_H
#define LATEX_CONFIG_H

#include <string>
#include <string_view>
#include <unordered_map>

namespace numsim::cas {

struct latex_config {
  std::unordered_map<std::size_t, std::string> tensor_fonts;

  static latex_config default_config() {
    latex_config cfg;
    cfg.tensor_fonts[4] = "\\mathbb";
    return cfg;
  }

  std::string font_for_rank(std::size_t rank) const {
    if (auto it = tensor_fonts.find(rank); it != tensor_fonts.end())
      return it->second;
    return "\\boldsymbol";
  }

  std::string format_tensor(std::string_view name, std::size_t rank) const {
    return font_for_rank(rank) + "{" + std::string(name) + "}";
  }
};

} // namespace numsim::cas

#endif // LATEX_CONFIG_H
