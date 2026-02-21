#include <array>
#include <benchmark/benchmark.h>
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <variant>
#include <vector>

static constexpr unsigned kNumTypes = 13;

std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type>
    random_pick(0, kNumTypes - 1);

template <std::size_t N> std::array<int, N> get_random_array() {
  std::array<int, N> item{};
  for (std::size_t i = 0; i < N; i++)
    item[i] = static_cast<int>(random_pick(rng));
  return item;
}

auto random_data{get_random_array<500>()};

using id_type = unsigned int;

namespace poly {

struct foo_base {
  explicit foo_base(id_type id) : m_id(id) {}
  virtual ~foo_base() = default;

  virtual id_type get_ID() const noexcept = 0;
  id_type get_ID_direct() const noexcept { return m_id; }

  std::unique_ptr<foo_base> _rhs;
  std::unique_ptr<foo_base> _lhs;
  std::vector<std::unique_ptr<foo_base>> data;
  std::string m_name;
  const id_type m_id;
};

struct foo1;
struct foo2;
struct foo3;
struct foo4;
struct foo5;
struct foo6;
struct foo7;
struct foo8;
struct foo9;
struct foo10;
struct foo11;
struct foo12;
struct foo13;

// Visitor template declaration
template <typename... Types> class Visitor;

template <typename T> class Visitor<T> {
public:
  virtual void operator()(T &visitable) noexcept = 0;
};

template <typename T, typename... Types>
class Visitor<T, Types...> : public Visitor<Types...> {
public:
  using Visitor<Types...>::operator();
  virtual void operator()(T &visitable) noexcept = 0;
};

template <typename... Types> class Visitable : public foo_base {
public:
  explicit Visitable(id_type id) : foo_base(id) {}
  virtual void accept(Visitor<Types...> &visitor) noexcept = 0;
};

template <typename Derived, typename... Types>
class VisitableImpl : public Visitable<Types...> {
public:
  explicit VisitableImpl(id_type id) : Visitable<Types...>(id) {}
  void accept(Visitor<Types...> &visitor) noexcept override {
    visitor(static_cast<Derived &>(*this));
  }
};

using visitor = Visitor<foo1, foo2, foo3, foo4, foo5, foo6, foo7, foo8, foo9,
                        foo10, foo11, foo12, foo13>;
using visitable = Visitable<foo1, foo2, foo3, foo4, foo5, foo6, foo7, foo8,
                            foo9, foo10, foo11, foo12, foo13>;
template <typename T>
using visitable_imp = VisitableImpl<T, foo1, foo2, foo3, foo4, foo5, foo6, foo7,
                                    foo8, foo9, foo10, foo11, foo12, foo13>;

#define POLY_DEFINE_FOO(N)                                                     \
  struct foo##N final : public visitable_imp<foo##N> {                         \
    foo##N() : visitable_imp<foo##N>(static_cast<id_type>((N) - 1)) {}         \
    id_type get_ID() const noexcept override {                                 \
      return static_cast<id_type>((N) - 1);                                    \
    }                                                                          \
  };

POLY_DEFINE_FOO(1)
POLY_DEFINE_FOO(2)
POLY_DEFINE_FOO(3)
POLY_DEFINE_FOO(4)
POLY_DEFINE_FOO(5)
POLY_DEFINE_FOO(6)
POLY_DEFINE_FOO(7)
POLY_DEFINE_FOO(8)
POLY_DEFINE_FOO(9)
POLY_DEFINE_FOO(10)
POLY_DEFINE_FOO(11)
POLY_DEFINE_FOO(12)
POLY_DEFINE_FOO(13)

#undef POLY_DEFINE_FOO

struct visitor_imp final : public visitor {
#define POLY_VISIT_NUM(N)                                                      \
  inline void operator()(foo##N &) noexcept override {                         \
    number = random_pick(rng);                                                 \
  }
  POLY_VISIT_NUM(1)
  POLY_VISIT_NUM(2)
  POLY_VISIT_NUM(3)
  POLY_VISIT_NUM(4)
  POLY_VISIT_NUM(5)
  POLY_VISIT_NUM(6)
  POLY_VISIT_NUM(7)
  POLY_VISIT_NUM(8)
  POLY_VISIT_NUM(9)
  POLY_VISIT_NUM(10)
  POLY_VISIT_NUM(11)
  POLY_VISIT_NUM(12)
  POLY_VISIT_NUM(13)
#undef POLY_VISIT_NUM
  double number{0};
};

struct visitor_id final : public visitor {
#define POLY_VISIT_ID(N)                                                       \
  inline void operator()(foo##N &) noexcept override {                         \
    number = static_cast<id_type>((N) - 1);                                    \
  }
  POLY_VISIT_ID(1)
  POLY_VISIT_ID(2)
  POLY_VISIT_ID(3)
  POLY_VISIT_ID(4)
  POLY_VISIT_ID(5)
  POLY_VISIT_ID(6)
  POLY_VISIT_ID(7)
  POLY_VISIT_ID(8)
  POLY_VISIT_ID(9)
  POLY_VISIT_ID(10)
  POLY_VISIT_ID(11)
  POLY_VISIT_ID(12)
  POLY_VISIT_ID(13)
#undef POLY_VISIT_ID
  id_type number{0};
};

// double-dispatch: selector chooses one of the 13 visitors (stack-owned)
struct visitor_simpilify_base : public visitor {
  double number{0.0};
  virtual ~visitor_simpilify_base() = default;
};

#define POLY_DEFINE_SIMPLIFY_VISITOR(N)                                        \
  struct visitor_simpilify_foo##N final : public visitor_simpilify_base {      \
    inline void bind(foo##N const &d) noexcept { m_data = &d; }                \
    /* all targets */                                                          \
    inline void operator()(foo1 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo2 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo3 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo4 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo5 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo6 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo7 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo8 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo9 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo10 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo11 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo12 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo13 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    foo##N const *m_data{nullptr};                                             \
  };

POLY_DEFINE_SIMPLIFY_VISITOR(1)
POLY_DEFINE_SIMPLIFY_VISITOR(2)
POLY_DEFINE_SIMPLIFY_VISITOR(3)
POLY_DEFINE_SIMPLIFY_VISITOR(4)
POLY_DEFINE_SIMPLIFY_VISITOR(5)
POLY_DEFINE_SIMPLIFY_VISITOR(6)
POLY_DEFINE_SIMPLIFY_VISITOR(7)
POLY_DEFINE_SIMPLIFY_VISITOR(8)
POLY_DEFINE_SIMPLIFY_VISITOR(9)
POLY_DEFINE_SIMPLIFY_VISITOR(10)
POLY_DEFINE_SIMPLIFY_VISITOR(11)
POLY_DEFINE_SIMPLIFY_VISITOR(12)
POLY_DEFINE_SIMPLIFY_VISITOR(13)

#undef POLY_DEFINE_SIMPLIFY_VISITOR

struct make_visitor_simpilify_fast final : public visitor {
  inline void operator()(foo1 &d) noexcept override {
    v1.bind(d);
    cur = &v1;
  }
  inline void operator()(foo2 &d) noexcept override {
    v2.bind(d);
    cur = &v2;
  }
  inline void operator()(foo3 &d) noexcept override {
    v3.bind(d);
    cur = &v3;
  }
  inline void operator()(foo4 &d) noexcept override {
    v4.bind(d);
    cur = &v4;
  }
  inline void operator()(foo5 &d) noexcept override {
    v5.bind(d);
    cur = &v5;
  }
  inline void operator()(foo6 &d) noexcept override {
    v6.bind(d);
    cur = &v6;
  }
  inline void operator()(foo7 &d) noexcept override {
    v7.bind(d);
    cur = &v7;
  }
  inline void operator()(foo8 &d) noexcept override {
    v8.bind(d);
    cur = &v8;
  }
  inline void operator()(foo9 &d) noexcept override {
    v9.bind(d);
    cur = &v9;
  }
  inline void operator()(foo10 &d) noexcept override {
    v10.bind(d);
    cur = &v10;
  }
  inline void operator()(foo11 &d) noexcept override {
    v11.bind(d);
    cur = &v11;
  }
  inline void operator()(foo12 &d) noexcept override {
    v12.bind(d);
    cur = &v12;
  }
  inline void operator()(foo13 &d) noexcept override {
    v13.bind(d);
    cur = &v13;
  }

  visitor_simpilify_base *cur{nullptr};

private:
  visitor_simpilify_foo1 v1;
  visitor_simpilify_foo2 v2;
  visitor_simpilify_foo3 v3;
  visitor_simpilify_foo4 v4;
  visitor_simpilify_foo5 v5;
  visitor_simpilify_foo6 v6;
  visitor_simpilify_foo7 v7;
  visitor_simpilify_foo8 v8;
  visitor_simpilify_foo9 v9;
  visitor_simpilify_foo10 v10;
  visitor_simpilify_foo11 v11;
  visitor_simpilify_foo12 v12;
  visitor_simpilify_foo13 v13;
};

// Variant-based double dispatch visitor types
#define POLY_DEFINE_VAR_VISITOR(N)                                             \
  struct visitor_simpilify_foo##N##_var final : public visitor {               \
    visitor_simpilify_foo##N##_var() = default;                                \
    explicit visitor_simpilify_foo##N##_var(foo##N &data) : m_data(&data) {}   \
    visitor_simpilify_foo##N##_var const &                                     \
    operator=(visitor_simpilify_foo##N##_var const &data) {                    \
      m_data = data.m_data;                                                    \
      return *this;                                                            \
    }                                                                          \
    inline void operator()(foo1 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo2 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo3 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo4 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo5 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo6 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo7 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo8 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo9 &) noexcept override {                         \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo10 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo11 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo12 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    inline void operator()(foo13 &) noexcept override {                        \
      number = random_pick(rng);                                               \
    }                                                                          \
    foo##N *m_data{nullptr};                                                   \
    double number{0};                                                          \
  };

POLY_DEFINE_VAR_VISITOR(1)
POLY_DEFINE_VAR_VISITOR(2)
POLY_DEFINE_VAR_VISITOR(3)
POLY_DEFINE_VAR_VISITOR(4)
POLY_DEFINE_VAR_VISITOR(5)
POLY_DEFINE_VAR_VISITOR(6)
POLY_DEFINE_VAR_VISITOR(7)
POLY_DEFINE_VAR_VISITOR(8)
POLY_DEFINE_VAR_VISITOR(9)
POLY_DEFINE_VAR_VISITOR(10)
POLY_DEFINE_VAR_VISITOR(11)
POLY_DEFINE_VAR_VISITOR(12)
POLY_DEFINE_VAR_VISITOR(13)

#undef POLY_DEFINE_VAR_VISITOR

struct make_visitor_simpilify_var final : public visitor {
  using var =
      std::variant<visitor_simpilify_foo1_var, visitor_simpilify_foo2_var,
                   visitor_simpilify_foo3_var, visitor_simpilify_foo4_var,
                   visitor_simpilify_foo5_var, visitor_simpilify_foo6_var,
                   visitor_simpilify_foo7_var, visitor_simpilify_foo8_var,
                   visitor_simpilify_foo9_var, visitor_simpilify_foo10_var,
                   visitor_simpilify_foo11_var, visitor_simpilify_foo12_var,
                   visitor_simpilify_foo13_var>;

  make_visitor_simpilify_var() : m_data() {}

  inline void operator()(foo1 &data) noexcept override {
    m_data = var{std::in_place_index<0>, visitor_simpilify_foo1_var{data}};
  }
  inline void operator()(foo2 &data) noexcept override {
    m_data = var{std::in_place_index<1>, visitor_simpilify_foo2_var{data}};
  }
  inline void operator()(foo3 &data) noexcept override {
    m_data = var{std::in_place_index<2>, visitor_simpilify_foo3_var{data}};
  }
  inline void operator()(foo4 &data) noexcept override {
    m_data = var{std::in_place_index<3>, visitor_simpilify_foo4_var{data}};
  }
  inline void operator()(foo5 &data) noexcept override {
    m_data = var{std::in_place_index<4>, visitor_simpilify_foo5_var{data}};
  }
  inline void operator()(foo6 &data) noexcept override {
    m_data = var{std::in_place_index<5>, visitor_simpilify_foo6_var{data}};
  }
  inline void operator()(foo7 &data) noexcept override {
    m_data = var{std::in_place_index<6>, visitor_simpilify_foo7_var{data}};
  }
  inline void operator()(foo8 &data) noexcept override {
    m_data = var{std::in_place_index<7>, visitor_simpilify_foo8_var{data}};
  }
  inline void operator()(foo9 &data) noexcept override {
    m_data = var{std::in_place_index<8>, visitor_simpilify_foo9_var{data}};
  }
  inline void operator()(foo10 &data) noexcept override {
    m_data = var{std::in_place_index<9>, visitor_simpilify_foo10_var{data}};
  }
  inline void operator()(foo11 &data) noexcept override {
    m_data = var{std::in_place_index<10>, visitor_simpilify_foo11_var{data}};
  }
  inline void operator()(foo12 &data) noexcept override {
    m_data = var{std::in_place_index<11>, visitor_simpilify_foo12_var{data}};
  }
  inline void operator()(foo13 &data) noexcept override {
    m_data = var{std::in_place_index<12>, visitor_simpilify_foo13_var{data}};
  }

  var m_data;
};

struct visitor_simpilify {
  explicit visitor_simpilify(visitable &data) : m_data(data) {}

#define POLY_VISIT_VAR(N)                                                      \
  inline void operator()(visitor_simpilify_foo##N##_var &data) noexcept {      \
    m_data.accept(data);                                                       \
    number = data.number;                                                      \
  }
  POLY_VISIT_VAR(1)
  POLY_VISIT_VAR(2)
  POLY_VISIT_VAR(3)
  POLY_VISIT_VAR(4)
  POLY_VISIT_VAR(5)
  POLY_VISIT_VAR(6)
  POLY_VISIT_VAR(7)
  POLY_VISIT_VAR(8)
  POLY_VISIT_VAR(9)
  POLY_VISIT_VAR(10)
  POLY_VISIT_VAR(11)
  POLY_VISIT_VAR(12)
  POLY_VISIT_VAR(13)
#undef POLY_VISIT_VAR

  visitable &m_data;
  double number{0};
};

std::vector<std::unique_ptr<foo_base>> data;

static std::unique_ptr<foo_base> make_foo(unsigned idx) {
  switch (idx) {
  case 0:
    return std::make_unique<foo1>();
  case 1:
    return std::make_unique<foo2>();
  case 2:
    return std::make_unique<foo3>();
  case 3:
    return std::make_unique<foo4>();
  case 4:
    return std::make_unique<foo5>();
  case 5:
    return std::make_unique<foo6>();
  case 6:
    return std::make_unique<foo7>();
  case 7:
    return std::make_unique<foo8>();
  case 8:
    return std::make_unique<foo9>();
  case 9:
    return std::make_unique<foo10>();
  case 10:
    return std::make_unique<foo11>();
  case 11:
    return std::make_unique<foo12>();
  case 12:
    return std::make_unique<foo13>();
  default:
    return std::make_unique<foo1>();
  }
}

void init() {
  if (data.empty()) {
    data.reserve(random_data.size());
    for (auto idx : random_data) {
      data.push_back(make_foo(static_cast<unsigned>(idx)));
    }
  }
}

static void init_data(benchmark::State &state) {
  for (auto _ : state) {
    init();
    benchmark::DoNotOptimize(data);
  }
}

static void get_id(benchmark::State &state) {
  init();
  for (auto _ : state) {
    id_type id{0};
    for (auto &ptr : data) {
      id += ptr->get_ID();
    }
    benchmark::DoNotOptimize(id);
  }
}

static void get_typeid(benchmark::State &state) {
  init();
  for (auto _ : state) {
    for (auto &ptr : data) {
      const std::size_t id = std::type_index(typeid(*ptr)).hash_code();
      benchmark::DoNotOptimize(id);
    }
  }
}

static void get_id_visitor(benchmark::State &state) {
  init();
  visitor_id visitor{};
  for (auto _ : state) {
    id_type id{0};
    for (auto &ptr : data) {
      static_cast<visitable &>(*ptr).accept(visitor);
      id += visitor.number;
    }
    benchmark::DoNotOptimize(id);
  }
}

static void get_number(benchmark::State &state) {
  init();
  visitor_imp visitor{};
  for (auto _ : state) {
    double number{0};
    for (auto &ptr : data) {
      static_cast<visitable &>(*ptr).accept(visitor);
      number += visitor.number;
    }
    benchmark::DoNotOptimize(number);
  }
}

static void double_dispatch(benchmark::State &state) {
  init();
  make_visitor_simpilify_fast selector{};

  for (auto _ : state) {
    double number = 0.0;

    for (auto &ptrA : data) {
      static_cast<visitable &>(*ptrA).accept(selector);
      auto *cur = selector.cur;

      for (auto &ptrB : data) {
        static_cast<visitable &>(*ptrB).accept(*cur);
        number += cur->number;
      }
    }

    benchmark::DoNotOptimize(number);
  }
}

static void double_dispatch_variant(benchmark::State &state) {
  init();
  make_visitor_simpilify_var visitor{};
  for (auto _ : state) {
    double number{0};
    for (auto &ptrA : data) {
      static_cast<visitable &>(*ptrA).accept(visitor);
      for (auto &ptrB : data) {
        visitor_simpilify visitorB(static_cast<visitable &>(*ptrB));
        std::visit(visitorB, visitor.m_data);
        number += visitorB.number;
      }
    }
    benchmark::DoNotOptimize(number);
  }
}

namespace lookup {

// Use 13x13 table now.
using Fn = double (*)() noexcept;

#define POLY_RULE(A, B)                                                        \
  static double r##A##_##B() noexcept {                                        \
    return static_cast<double>(random_pick(rng));                              \
  }

POLY_RULE(0, 0);
POLY_RULE(0, 1);
POLY_RULE(0, 2);
POLY_RULE(0, 3);
POLY_RULE(0, 4);
POLY_RULE(0, 5);
POLY_RULE(0, 6);
POLY_RULE(0, 7);
POLY_RULE(0, 8);
POLY_RULE(0, 9);
POLY_RULE(0, 10);
POLY_RULE(0, 11);
POLY_RULE(0, 12);
POLY_RULE(1, 0);
POLY_RULE(1, 1);
POLY_RULE(1, 2);
POLY_RULE(1, 3);
POLY_RULE(1, 4);
POLY_RULE(1, 5);
POLY_RULE(1, 6);
POLY_RULE(1, 7);
POLY_RULE(1, 8);
POLY_RULE(1, 9);
POLY_RULE(1, 10);
POLY_RULE(1, 11);
POLY_RULE(1, 12);
POLY_RULE(2, 0);
POLY_RULE(2, 1);
POLY_RULE(2, 2);
POLY_RULE(2, 3);
POLY_RULE(2, 4);
POLY_RULE(2, 5);
POLY_RULE(2, 6);
POLY_RULE(2, 7);
POLY_RULE(2, 8);
POLY_RULE(2, 9);
POLY_RULE(2, 10);
POLY_RULE(2, 11);
POLY_RULE(2, 12);
POLY_RULE(3, 0);
POLY_RULE(3, 1);
POLY_RULE(3, 2);
POLY_RULE(3, 3);
POLY_RULE(3, 4);
POLY_RULE(3, 5);
POLY_RULE(3, 6);
POLY_RULE(3, 7);
POLY_RULE(3, 8);
POLY_RULE(3, 9);
POLY_RULE(3, 10);
POLY_RULE(3, 11);
POLY_RULE(3, 12);
POLY_RULE(4, 0);
POLY_RULE(4, 1);
POLY_RULE(4, 2);
POLY_RULE(4, 3);
POLY_RULE(4, 4);
POLY_RULE(4, 5);
POLY_RULE(4, 6);
POLY_RULE(4, 7);
POLY_RULE(4, 8);
POLY_RULE(4, 9);
POLY_RULE(4, 10);
POLY_RULE(4, 11);
POLY_RULE(4, 12);
POLY_RULE(5, 0);
POLY_RULE(5, 1);
POLY_RULE(5, 2);
POLY_RULE(5, 3);
POLY_RULE(5, 4);
POLY_RULE(5, 5);
POLY_RULE(5, 6);
POLY_RULE(5, 7);
POLY_RULE(5, 8);
POLY_RULE(5, 9);
POLY_RULE(5, 10);
POLY_RULE(5, 11);
POLY_RULE(5, 12);
POLY_RULE(6, 0);
POLY_RULE(6, 1);
POLY_RULE(6, 2);
POLY_RULE(6, 3);
POLY_RULE(6, 4);
POLY_RULE(6, 5);
POLY_RULE(6, 6);
POLY_RULE(6, 7);
POLY_RULE(6, 8);
POLY_RULE(6, 9);
POLY_RULE(6, 10);
POLY_RULE(6, 11);
POLY_RULE(6, 12);
POLY_RULE(7, 0);
POLY_RULE(7, 1);
POLY_RULE(7, 2);
POLY_RULE(7, 3);
POLY_RULE(7, 4);
POLY_RULE(7, 5);
POLY_RULE(7, 6);
POLY_RULE(7, 7);
POLY_RULE(7, 8);
POLY_RULE(7, 9);
POLY_RULE(7, 10);
POLY_RULE(7, 11);
POLY_RULE(7, 12);
POLY_RULE(8, 0);
POLY_RULE(8, 1);
POLY_RULE(8, 2);
POLY_RULE(8, 3);
POLY_RULE(8, 4);
POLY_RULE(8, 5);
POLY_RULE(8, 6);
POLY_RULE(8, 7);
POLY_RULE(8, 8);
POLY_RULE(8, 9);
POLY_RULE(8, 10);
POLY_RULE(8, 11);
POLY_RULE(8, 12);
POLY_RULE(9, 0);
POLY_RULE(9, 1);
POLY_RULE(9, 2);
POLY_RULE(9, 3);
POLY_RULE(9, 4);
POLY_RULE(9, 5);
POLY_RULE(9, 6);
POLY_RULE(9, 7);
POLY_RULE(9, 8);
POLY_RULE(9, 9);
POLY_RULE(9, 10);
POLY_RULE(9, 11);
POLY_RULE(9, 12);
POLY_RULE(10, 0);
POLY_RULE(10, 1);
POLY_RULE(10, 2);
POLY_RULE(10, 3);
POLY_RULE(10, 4);
POLY_RULE(10, 5);
POLY_RULE(10, 6);
POLY_RULE(10, 7);
POLY_RULE(10, 8);
POLY_RULE(10, 9);
POLY_RULE(10, 10);
POLY_RULE(10, 11);
POLY_RULE(10, 12);
POLY_RULE(11, 0);
POLY_RULE(11, 1);
POLY_RULE(11, 2);
POLY_RULE(11, 3);
POLY_RULE(11, 4);
POLY_RULE(11, 5);
POLY_RULE(11, 6);
POLY_RULE(11, 7);
POLY_RULE(11, 8);
POLY_RULE(11, 9);
POLY_RULE(11, 10);
POLY_RULE(11, 11);
POLY_RULE(11, 12);
POLY_RULE(12, 0);
POLY_RULE(12, 1);
POLY_RULE(12, 2);
POLY_RULE(12, 3);
POLY_RULE(12, 4);
POLY_RULE(12, 5);
POLY_RULE(12, 6);
POLY_RULE(12, 7);
POLY_RULE(12, 8);
POLY_RULE(12, 9);
POLY_RULE(12, 10);
POLY_RULE(12, 11);
POLY_RULE(12, 12);

#undef POLY_RULE

static Fn table[13][13] = {
    {&r0_0, &r0_1, &r0_2, &r0_3, &r0_4, &r0_5, &r0_6, &r0_7, &r0_8, &r0_9,
     &r0_10, &r0_11, &r0_12},
    {&r1_0, &r1_1, &r1_2, &r1_3, &r1_4, &r1_5, &r1_6, &r1_7, &r1_8, &r1_9,
     &r1_10, &r1_11, &r1_12},
    {&r2_0, &r2_1, &r2_2, &r2_3, &r2_4, &r2_5, &r2_6, &r2_7, &r2_8, &r2_9,
     &r2_10, &r2_11, &r2_12},
    {&r3_0, &r3_1, &r3_2, &r3_3, &r3_4, &r3_5, &r3_6, &r3_7, &r3_8, &r3_9,
     &r3_10, &r3_11, &r3_12},
    {&r4_0, &r4_1, &r4_2, &r4_3, &r4_4, &r4_5, &r4_6, &r4_7, &r4_8, &r4_9,
     &r4_10, &r4_11, &r4_12},
    {&r5_0, &r5_1, &r5_2, &r5_3, &r5_4, &r5_5, &r5_6, &r5_7, &r5_8, &r5_9,
     &r5_10, &r5_11, &r5_12},
    {&r6_0, &r6_1, &r6_2, &r6_3, &r6_4, &r6_5, &r6_6, &r6_7, &r6_8, &r6_9,
     &r6_10, &r6_11, &r6_12},
    {&r7_0, &r7_1, &r7_2, &r7_3, &r7_4, &r7_5, &r7_6, &r7_7, &r7_8, &r7_9,
     &r7_10, &r7_11, &r7_12},
    {&r8_0, &r8_1, &r8_2, &r8_3, &r8_4, &r8_5, &r8_6, &r8_7, &r8_8, &r8_9,
     &r8_10, &r8_11, &r8_12},
    {&r9_0, &r9_1, &r9_2, &r9_3, &r9_4, &r9_5, &r9_6, &r9_7, &r9_8, &r9_9,
     &r9_10, &r9_11, &r9_12},
    {&r10_0, &r10_1, &r10_2, &r10_3, &r10_4, &r10_5, &r10_6, &r10_7, &r10_8,
     &r10_9, &r10_10, &r10_11, &r10_12},
    {&r11_0, &r11_1, &r11_2, &r11_3, &r11_4, &r11_5, &r11_6, &r11_7, &r11_8,
     &r11_9, &r11_10, &r11_11, &r11_12},
    {&r12_0, &r12_1, &r12_2, &r12_3, &r12_4, &r12_5, &r12_6, &r12_7, &r12_8,
     &r12_9, &r12_10, &r12_11, &r12_12},
};

static void double_dispatch_table_vcall(benchmark::State &state) {
  poly::init();

  for (auto _ : state) {
    double sum = 0.0;

    for (auto &a : poly::data) {
      const auto ia = static_cast<std::size_t>(a->get_ID()); // virtual
      for (auto &b : poly::data) {
        const auto ib = static_cast<std::size_t>(b->get_ID()); // virtual
        sum += table[ia][ib]();
      }
    }
    benchmark::DoNotOptimize(sum);
  }
}

static void double_dispatch_table_direct_id_vcall(benchmark::State &state) {
  poly::init();

  for (auto _ : state) {
    double sum = 0.0;

    for (auto &a : poly::data) {
      const auto ia =
          static_cast<std::size_t>(a->get_ID_direct()); // non-virtual
      for (auto &b : poly::data) {
        const auto ib =
            static_cast<std::size_t>(b->get_ID_direct()); // non-virtual
        sum += table[ia][ib]();
      }
    }
    benchmark::DoNotOptimize(sum);
  }
}

} // namespace lookup

} // namespace poly

namespace var {

struct foo_base {
  virtual ~foo_base() = default;
};

#define VAR_DEFINE_FOO(N)                                                      \
  struct foo##N final : public foo_base {                                      \
    constexpr id_type get_ID() const noexcept {                                \
      return static_cast<id_type>((N) - 1);                                    \
    }                                                                          \
  };

VAR_DEFINE_FOO(1)
VAR_DEFINE_FOO(2)
VAR_DEFINE_FOO(3)
VAR_DEFINE_FOO(4)
VAR_DEFINE_FOO(5)
VAR_DEFINE_FOO(6)
VAR_DEFINE_FOO(7)
VAR_DEFINE_FOO(8)
VAR_DEFINE_FOO(9)
VAR_DEFINE_FOO(10)
VAR_DEFINE_FOO(11)
VAR_DEFINE_FOO(12)
VAR_DEFINE_FOO(13)

#undef VAR_DEFINE_FOO

using variant = std::variant<foo1, foo2, foo3, foo4, foo5, foo6, foo7, foo8,
                             foo9, foo10, foo11, foo12, foo13>;

std::vector<std::unique_ptr<variant>> data;

void init() {
  if (data.empty()) {
    data.reserve(random_data.size());
    for (auto idx : random_data) {
      switch (static_cast<unsigned>(idx)) {
      case 0:
        data.push_back(std::make_unique<variant>(foo1{}));
        break;
      case 1:
        data.push_back(std::make_unique<variant>(foo2{}));
        break;
      case 2:
        data.push_back(std::make_unique<variant>(foo3{}));
        break;
      case 3:
        data.push_back(std::make_unique<variant>(foo4{}));
        break;
      case 4:
        data.push_back(std::make_unique<variant>(foo5{}));
        break;
      case 5:
        data.push_back(std::make_unique<variant>(foo6{}));
        break;
      case 6:
        data.push_back(std::make_unique<variant>(foo7{}));
        break;
      case 7:
        data.push_back(std::make_unique<variant>(foo8{}));
        break;
      case 8:
        data.push_back(std::make_unique<variant>(foo9{}));
        break;
      case 9:
        data.push_back(std::make_unique<variant>(foo10{}));
        break;
      case 10:
        data.push_back(std::make_unique<variant>(foo11{}));
        break;
      case 11:
        data.push_back(std::make_unique<variant>(foo12{}));
        break;
      case 12:
        data.push_back(std::make_unique<variant>(foo13{}));
        break;
      default:
        data.push_back(std::make_unique<variant>(foo1{}));
        break;
      }
    }
  }
}

struct visitor_id {
  id_type operator()(foo1 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo2 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo3 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo4 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo5 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo6 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo7 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo8 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo9 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo10 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo11 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo12 const &x) const noexcept { return x.get_ID(); }
  id_type operator()(foo13 const &x) const noexcept { return x.get_ID(); }
};

struct visitor_number {
  double operator()(foo1 const &) const noexcept { return random_pick(rng); }
  double operator()(foo2 const &) const noexcept { return random_pick(rng); }
  double operator()(foo3 const &) const noexcept { return random_pick(rng); }
  double operator()(foo4 const &) const noexcept { return random_pick(rng); }
  double operator()(foo5 const &) const noexcept { return random_pick(rng); }
  double operator()(foo6 const &) const noexcept { return random_pick(rng); }
  double operator()(foo7 const &) const noexcept { return random_pick(rng); }
  double operator()(foo8 const &) const noexcept { return random_pick(rng); }
  double operator()(foo9 const &) const noexcept { return random_pick(rng); }
  double operator()(foo10 const &) const noexcept { return random_pick(rng); }
  double operator()(foo11 const &) const noexcept { return random_pick(rng); }
  double operator()(foo12 const &) const noexcept { return random_pick(rng); }
  double operator()(foo13 const &) const noexcept { return random_pick(rng); }
};

static void init_data(benchmark::State &state) {
  for (auto _ : state) {
    init();
    benchmark::DoNotOptimize(data);
  }
}

static void get_id(benchmark::State &state) {
  init();
  for (auto _ : state) {
    id_type id{0};
    for (auto &ptr : data)
      id += std::visit(visitor_id{}, *ptr);
    benchmark::DoNotOptimize(id);
  }
}

static void get_number(benchmark::State &state) {
  init();
  for (auto _ : state) {
    double s{0};
    for (auto &ptr : data)
      s += std::visit(visitor_number{}, *ptr);
    benchmark::DoNotOptimize(s);
  }
}

static void get_number_constexp(benchmark::State &state) {
  init();
  auto func = [](auto const &) { return random_pick(rng); };
  for (auto _ : state) {
    double s{0};
    for (auto &ptr : data)
      s += std::visit(func, *ptr);
    benchmark::DoNotOptimize(s);
  }
}

static void double_dispatch(benchmark::State &state) {
  init();
  // Emulates "A selects a visitor" then B is visited with it.
  // For variant, easiest is binary visit directly:
  for (auto _ : state) {
    double sum = 0.0;
    for (auto &a : data) {
      for (auto &b : data) {
        sum += std::visit(
            [](auto &, auto &) { return double(random_pick(rng)); }, *a, *b);
      }
    }
    benchmark::DoNotOptimize(sum);
  }
}

} // namespace var

BENCHMARK(poly::init_data);
BENCHMARK(poly::get_id);
BENCHMARK(poly::get_id_visitor);
BENCHMARK(poly::get_typeid);
BENCHMARK(poly::get_number);
BENCHMARK(poly::double_dispatch);
BENCHMARK(poly::double_dispatch_variant);
BENCHMARK(poly::lookup::double_dispatch_table_vcall);
BENCHMARK(poly::lookup::double_dispatch_table_direct_id_vcall);

BENCHMARK(var::init_data);
BENCHMARK(var::get_id);
BENCHMARK(var::get_number);
BENCHMARK(var::get_number_constexp);
BENCHMARK(var::double_dispatch);

BENCHMARK_MAIN();
