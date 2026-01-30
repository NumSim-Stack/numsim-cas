#ifndef TAG_INVOKE_H
#define TAG_INVOKE_H

#include <utility>

namespace numsim::cas::detail {

// ADL anchor
void tag_invoke();

template <class Tag, class... Args>
concept tag_invocable = requires(Tag tag, Args &&...args) {
  tag_invoke(tag, std::forward<Args>(args)...);
};

template <class Tag, class... Args>
using tag_invoke_result_t =
    decltype(tag_invoke(std::declval<Tag>(), std::declval<Args>()...));

} // namespace numsim::cas::detail

#endif // TAG_INVOKE_H
