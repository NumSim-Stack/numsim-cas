#ifndef TENSOR_CONSTANT_H
#define TENSOR_CONSTANT_H

#include "numsim_cas_type_traits.h"

namespace numsim::cas {

class tensor_constant final
    : public expression_crtp<tensor_constant, tensor_expression> {
public:
  using base = expression_crtp<tensor_constant, tensor_expression>;

  tensor_constant() : m_data() {}
  template <typename TensorData>
  explicit tensor_constant(TensorData const &data) : m_data(data) {
    hash_combine(this->m_hash_value, base::get_id());
    hash_combine(this->m_hash_value, data);
  }
  explicit tensor_constant(tensor_constant const &data)
      : base(static_cast<base const &>(data)), m_data(data.m_data) {}
  explicit tensor_constant(tensor_constant &&data)
      : base(static_cast<base &&>(data)), m_data(std::forward(data.m_data)) {}

  //  template<typename T, std::enable_if_t<std::is_same_v<ValueType,
  //  std::remove_cvref_t<T>>, bool> = true> constexpr inline const auto&
  //  operator=(T && value){
  //    m_data = value;
  //    return *this;
  //  }

  constexpr inline const auto &operator()() const { return m_data; }

  //  constexpr inline auto& operator()(){
  //    return m_data;
  //  }
private:
  std::any m_data;
};

} // namespace numsim::cas

#endif // TENSOR_CONSTANT_H
