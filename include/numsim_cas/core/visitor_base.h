#ifndef VISITOR_BASE_H
#define VISITOR_BASE_H

#include <cassert>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <numsim_cas/type_list.h>

namespace numsim::cas {

// ---------------- returning visitor ----------------
// ReturnT is something like expression_holder<scalar_expression>
template <typename ReturnT, typename... Types> class visitor_return;

template <typename ReturnT, typename T> class visitor_return<ReturnT, T> {
public:
  using return_type = ReturnT;
  virtual ~visitor_return() = default;
  virtual return_type operator()(T const &visitable) = 0;
};

template <typename ReturnT, typename T, typename... Types>
class visitor_return<ReturnT, T, Types...>
    : public visitor_return<ReturnT, Types...> {
public:
  using return_type = ReturnT;
  using visitor_return<ReturnT, Types...>::operator();
  virtual ~visitor_return() = default;
  virtual return_type operator()(T const &visitable) = 0;
};

// ---------------- mutating visitor ----------------
template <typename... Types> class visitor;

template <typename T> class visitor<T> {
public:
  virtual ~visitor() = default;
  virtual void operator()(T &visitable) = 0;
};

template <typename T, typename... Types>
class visitor<T, Types...> : public visitor<Types...> {
public:
  using visitor<Types...>::operator();
  virtual ~visitor() = default;
  virtual void operator()(T &visitable) = 0;
};

// ---------------- const-node visitor (stateful visitor) ----------------
template <typename... Types> class visitor_const;

template <typename T> class visitor_const<T> {
public:
  virtual ~visitor_const() = default;
  virtual void operator()(T const &visitable) = 0;
};

template <typename T, typename... Types>
class visitor_const<T, Types...> : public visitor_const<Types...> {
public:
  using visitor_const<Types...>::operator();
  virtual ~visitor_const() = default;
  virtual void operator()(T const &visitable) = 0;
};

// ---------------- visitable / visitable_impl ----------------
template <typename Base, typename... Types> class visitable : public Base {
public:
  using return_type = expression_holder<Base>;

  using Base::Base;
  template <typename... Args>
  visitable(Args &&...args) : Base(std::forward<Args>(args)...) {}
  visitable() {}
  virtual ~visitable() = default;

  virtual void accept(visitor<Types...> &v) = 0;
  virtual void accept(visitor_const<Types...> &v) const = 0;

  // NOTE: visitor_return now needs ReturnT explicitly
  virtual return_type
  accept(visitor_return<return_type, Types...> &v) const = 0;
};

template <typename Base, typename Derived, typename... Types>
class visitable_impl : public visitable<Base, Types...> {
public:
  using return_type = typename visitable<Base, Types...>::return_type;
  using expr_holder_t = expression_holder<Base>;

  template <typename... Args>
  visitable_impl(Args &&...args)
      : visitable<Base, Types...>(std::forward<Args>(args)...) {}
  visitable_impl() = default;
  using visitable<Base, Types...>::visitable;
  virtual ~visitable_impl() = default;

  void accept(visitor<Types...> &v) override {
    v(static_cast<Derived &>(*this));
  }

  void accept(visitor_const<Types...> &v) const override {
    v(static_cast<Derived const &>(*this));
  }

  return_type accept(visitor_return<return_type, Types...> &v) const override {
    return v(static_cast<Derived const &>(*this));
  }

  [[nodiscard]] static type_id get_id() noexcept {
    return get_index<typename get_derived<Derived>::derived_type, Base>::index;
  }

  [[nodiscard]] type_id id() const noexcept override { return get_id(); }

protected:
  bool equals_same_type(expression const &rhs) const noexcept override {
    assert(dynamic_cast<Derived const *>(&rhs) != nullptr);
    return static_cast<Derived const &>(*this) ==
           static_cast<Derived const &>(rhs);
  }

  bool less_than_same_type(expression const &rhs) const noexcept override {
    assert(dynamic_cast<Derived const *>(&rhs) != nullptr);
    return static_cast<Derived const &>(*this) <
           static_cast<Derived const &>(rhs);
  }
};

} // namespace numsim::cas

#endif // VISITOR_BASE_H
