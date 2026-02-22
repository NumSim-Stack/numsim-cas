#ifndef TENSOR_DATA_H
#define TENSOR_DATA_H

#include <cstdlib>
#include <memory>
#include <string>
#include <tmech/tmech.h>

namespace numsim::cas {

template <typename ValueType> class tensor_data_base {
public:
  tensor_data_base() {}
  tensor_data_base(tensor_data_base const &) = delete;
  tensor_data_base(tensor_data_base &&) = delete;
  virtual ~tensor_data_base() = default;
  const tensor_data_base &operator=(tensor_data_base const &) = delete;

  [[nodiscard]] virtual std::size_t dim() const noexcept = 0;
  [[nodiscard]] virtual std::size_t rank() const noexcept = 0;
  [[nodiscard]] virtual const ValueType *raw_data() const noexcept = 0;
  [[nodiscard]] virtual ValueType *raw_data() noexcept = 0;
  virtual void print(std::ostream &out) noexcept = 0;
  template <typename T>
  inline friend std::ostream &operator<<(std::ostream &out,
                                         tensor_data_base<T> &);
};

template <typename ValueType>
inline std::ostream &operator<<(std::ostream &out,
                                tensor_data_base<ValueType> &tensor) {
  tensor.print(out);
  return out;
}

template <typename ValueType, std::size_t Dim, std::size_t Rank>
class tensor_data final : public tensor_data_base<ValueType> {
public:
  using data_type = tmech::tensor<ValueType, Dim, Rank>;
  tensor_data() : m_data() {}

  explicit tensor_data(data_type const &data)
      : tensor_data_base<ValueType>(), m_data(data) {}

  [[nodiscard]] std::size_t dim() const noexcept final {
    return Dim;
  }

  [[nodiscard]] std::size_t rank() const noexcept final {
    return Rank;
  }

  [[nodiscard]] const auto &data() const { return m_data; }

  [[nodiscard]] auto &data() { return m_data; }

  [[nodiscard]] const ValueType *raw_data() const noexcept final {
    return m_data.raw_data();
  }

  [[nodiscard]] ValueType *raw_data() noexcept final {
    return m_data.raw_data();
  }

  void print(std::ostream &out) noexcept final {
    out << m_data;
  }

private:
  data_type m_data;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_H
