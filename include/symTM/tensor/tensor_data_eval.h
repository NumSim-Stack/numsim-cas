#ifndef TENSOR_DATA_EVAL_H
#define TENSOR_DATA_EVAL_H

#include <cstdlib>

namespace symTM {

template <typename Derived, typename ValueType, std::size_t MaxDim, std::size_t MaxRank,
          std::size_t MaxArgs>
class tensor_data_eval {
public:
  tensor_data_eval() = default;
  tensor_data_eval(tensor_data_eval const &) = delete;
  tensor_data_eval(tensor_data_eval &&) = delete;
  const tensor_data_eval &operator=(tensor_data_eval const &) = delete;
  virtual ~tensor_data_eval() = default;

  [[nodiscard]] Derived &convert() noexcept {
    return static_cast<Derived &>(*this);
  }

  template <typename... Args>
  [[nodiscard]] auto evaluate(std::size_t dim, Args... args) {
    static_assert(sizeof...(args) == MaxArgs, "");
    return eval_dim<1>(dim, args...);
  }

protected:
  static constexpr auto _MaxDim{MaxDim};
  static constexpr auto _MaxRank{MaxRank};

private:
  template <std::size_t Dim, std::size_t RankIter,
            std::size_t... Ranks, typename... Args>
  [[nodiscard]] constexpr inline auto rank_loop(std::size_t rank,
                                                Args... args) {
    if constexpr (RankIter <= MaxRank) {
      if (rank == RankIter) {
        if constexpr (sizeof...(Ranks) + 1 == MaxArgs) {
          return rank_loop<Dim, RankIter, Ranks...>(args...);
        } else {
          return rank_loop<Dim, 1, RankIter, Ranks...>(args...);
        }
      } else {
        return rank_loop<Dim, RankIter + 1, Ranks...>(rank, args...);
      }
    } else {
      return convert().missmatch(Dim, RankIter, Ranks..., args...);
    }
  }

  template <std::size_t Dim, std::size_t... Ranks>
  [[nodiscard]] constexpr inline auto rank_loop() {
    return convert().template evaluate_imp<Dim, Ranks...>();
  }

  template <std::size_t DimIter, typename... Args>
  [[nodiscard]] constexpr inline auto eval_dim(std::size_t dim, Args... args) {
    if constexpr (DimIter <= MaxDim) {
      if (dim == DimIter) {
        return rank_loop<DimIter, 1>(args...);
      } else {
        return eval_dim<DimIter + 1>(dim, args...);
      }
    } else {
      return convert().missmatch(dim, args...);
    }
  }
};

}

#endif // TENSOR_DATA_EVAL_H
