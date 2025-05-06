#include <iostream>
#include <benchmark/benchmark.h>
#include <variant>
#include <array>
#include <random>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
#include <experimental/type_traits>
#include <typeindex>


std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> random_pick(0,3);

template <std::size_t N>
std::array<int, N> get_random_array() {
  std::array<int, N> item;
  for (int i = 0 ; i < N; i++)
    item[i] = random_pick(rng);
  return item;
}

auto random_data{get_random_array<5>()};

using id_type = unsigned int;


namespace poly {

struct foo_base{
  foo_base() = default;
  virtual~foo_base() = default;
  virtual id_type get_ID() const noexcept= 0;

  std::unique_ptr<foo_base> _rhs;
  std::unique_ptr<foo_base> _lhs;
  std::vector<std::unique_ptr<foo_base>> data;
  std::string m_name;
};


struct foo1;
struct foo2;
struct foo3;
struct foo4;

// Visitor template declaration
template<typename... Types>
class Visitor;

// specialization for single type
template<typename T>
class Visitor<T> {
public:
  virtual void operator()(T & visitable) noexcept = 0;
};

// specialization for multiple types
template<typename T, typename... Types>
class Visitor<T, Types...> : public Visitor<Types...> {
public:
  // promote the function(s) from the base class
  using Visitor<Types...>::operator();

  virtual void operator()(T & visitable) noexcept = 0;
};

template<typename... Types>
class Visitable : public foo_base{
public:
  virtual void accept(Visitor<Types...>& visitor) noexcept = 0;
};

template<typename Derived, typename... Types>
class VisitableImpl : public Visitable<Types...> {
public:
  void accept(Visitor<Types...>& visitor)noexcept override {
    visitor(static_cast<Derived&>(*this));
  }
};

using visitor = Visitor<foo1,foo2,foo3,foo4>;
using visitable = Visitable<foo1,foo2,foo3,foo4>;
template<typename T>
using visitable_imp = VisitableImpl<T, foo1,foo2,foo3,foo4>;


struct foo1 final : public visitable_imp<foo1>
{
  id_type get_ID() const noexcept override {return 0;}
};
struct foo2 final : public visitable_imp<foo2>
{
  id_type get_ID() const noexcept override {return 1;}
};
struct foo3 final : public visitable_imp<foo3>
{
  id_type get_ID() const noexcept override {return 2;}
};
struct foo4 final : public visitable_imp<foo4>
{
  id_type get_ID() const noexcept override {return 3;}
};

struct visitor_imp final : public visitor
{
  inline void operator()(foo1&)noexcept override{number=rand();};
  inline void operator()(foo2&)noexcept override{number=rand();};
  inline void operator()(foo3&)noexcept override{number=rand();};
  inline void operator()(foo4&)noexcept override{number=rand();};
  double number{0};
};

struct visitor_id final : public visitor
{
  inline void operator()(foo1&)noexcept override{number=0;};
  inline void operator()(foo2&)noexcept override{number=1;};
  inline void operator()(foo3&)noexcept override{number=2;};
  inline void operator()(foo4&)noexcept override{number=3;};
  id_type number{0};
};

struct visitor_simpilify_base
{
  virtual void apply(visitable &) = 0;
  double number{0};
};

struct visitor_simpilify_foo1 final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo1(foo1 const& data):m_data(data){}
  explicit visitor_simpilify_foo1(visitor_simpilify_foo1 const& data):m_data(data.m_data){}
  virtual void apply(visitable &data)override{data.accept(*this);}
  inline void operator()(foo1&)noexcept override final{number=rand();};
  inline void operator()(foo2&)noexcept override final{number=rand();};
  inline void operator()(foo3&)noexcept override final{number=rand();};
  inline void operator()(foo4&)noexcept override final{number=rand();};
  foo1 const& m_data;
};

struct visitor_simpilify_foo2 final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo2(foo2 const& data):m_data(data){}
  virtual void apply(visitable &data)override{data.accept(*this);}
  inline void operator()(foo1&)noexcept override final{number=rand();};
  inline void operator()(foo2&)noexcept override final{number=rand();};
  inline void operator()(foo3&)noexcept override final{number=rand();};
  inline void operator()(foo4&)noexcept override final{number=rand();};
  foo2 const& m_data;
};

struct visitor_simpilify_foo3 final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo3(foo3 const& data):m_data(data){}
  virtual void apply(visitable &data)override{data.accept(*this);}
  inline void operator()(foo1&)noexcept override final{number=rand();};
  inline void operator()(foo2&)noexcept override final{number=rand();};
  inline void operator()(foo3&)noexcept override final{number=rand();};
  inline void operator()(foo4&)noexcept override final{number=rand();};
  foo3 const& m_data;
};

struct visitor_simpilify_foo4 final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo4(foo4 const& data):m_data(data){}
  virtual void apply(visitable &data)override{data.accept(*this);}
  inline void operator()(foo1&)noexcept override final{number=rand();};
  inline void operator()(foo2&)noexcept override final{number=rand();};
  inline void operator()(foo3&)noexcept override final{number=rand();};
  inline void operator()(foo4&)noexcept override final{number=rand();};
  foo4 const& m_data;
};


struct make_visitor_simpilify final : public visitor
{
  inline void operator()(foo1&data)noexcept override{m_data = std::make_unique<visitor_simpilify_foo1>(data);};
  inline void operator()(foo2&data)noexcept override{m_data = std::make_unique<visitor_simpilify_foo2>(data);};
  inline void operator()(foo3&data)noexcept override{m_data = std::make_unique<visitor_simpilify_foo3>(data);};
  inline void operator()(foo4&data)noexcept override{m_data = std::make_unique<visitor_simpilify_foo4>(data);};
  std::unique_ptr<visitor_simpilify_base> m_data;
};

struct visitor_simpilify_foo1_var final : public visitor
{
  visitor_simpilify_foo1_var(){}
  explicit visitor_simpilify_foo1_var(foo1 & data):m_data(&data){}
  visitor_simpilify_foo1_var const& operator=(visitor_simpilify_foo1_var const& data){
    m_data = data.m_data;
    return *this;
  }

  inline void operator()(foo1&)noexcept override{number=rand();};
  inline void operator()(foo2&)noexcept override{number=rand();};
  inline void operator()(foo3&)noexcept override{number=rand();};
  inline void operator()(foo4&)noexcept override{number=rand();};
  foo1 * m_data;
  double number;
};

struct visitor_simpilify_foo2_var final : public visitor
{
  visitor_simpilify_foo2_var(){}
  explicit visitor_simpilify_foo2_var(foo2 & data):m_data(&data){}
  visitor_simpilify_foo2_var const& operator=(visitor_simpilify_foo2_var const& data){
    m_data = data.m_data;
    return *this;
  }

  inline void operator()(foo1&)noexcept override{number=rand();};
  inline void operator()(foo2&)noexcept override{number=rand();};
  inline void operator()(foo3&)noexcept override{number=rand();};
  inline void operator()(foo4&)noexcept override{number=rand();};
  foo2 * m_data;
  double number;
};

struct visitor_simpilify_foo3_var final : public visitor
{
  visitor_simpilify_foo3_var(){}
  explicit visitor_simpilify_foo3_var(foo3 & data):m_data(&data){}
  visitor_simpilify_foo3_var const& operator=(visitor_simpilify_foo3_var const& data){
    m_data = data.m_data;
    return *this;
  }

  inline void operator()(foo1&)noexcept override{number=rand();};
  inline void operator()(foo2&)noexcept override{number=rand();};
  inline void operator()(foo3&)noexcept override{number=rand();};
  inline void operator()(foo4&)noexcept override{number=rand();};
  foo3 * m_data;
  double number;
};

struct visitor_simpilify_foo4_var final : public visitor
{
  visitor_simpilify_foo4_var(){}
  explicit visitor_simpilify_foo4_var(foo4 & data):m_data(&data){}
  visitor_simpilify_foo4_var const& operator=(visitor_simpilify_foo4_var const& data){
    m_data = data.m_data;
    return *this;
  }

  inline void operator()(foo1&)noexcept override{number=rand();};
  inline void operator()(foo2&)noexcept override{number=rand();};
  inline void operator()(foo3&)noexcept override{number=rand();};
  inline void operator()(foo4&)noexcept override{number=rand();};
  foo4 * m_data;
  double number;
};

struct make_visitor_simpilify_var final : public visitor
{
  using var = std::variant<visitor_simpilify_foo1_var,visitor_simpilify_foo2_var,visitor_simpilify_foo3_var,visitor_simpilify_foo4_var>;//,visitor_simpilify_foo2,visitor_simpilify_foo3,visitor_simpilify_foo4>;
  make_visitor_simpilify_var():m_data(){}

  inline void operator()(foo1&data)noexcept override{m_data = var{std::in_place_index<0>,visitor_simpilify_foo1_var{data}};};
  inline void operator()(foo2&data)noexcept override{m_data = var{std::in_place_index<1>,visitor_simpilify_foo2_var{data}};};
  inline void operator()(foo3&data)noexcept override{m_data = var{std::in_place_index<2>,visitor_simpilify_foo3_var{data}};};
  inline void operator()(foo4&data)noexcept override{m_data = var{std::in_place_index<3>,visitor_simpilify_foo4_var{data}};};
  var m_data;
};


struct visitor_simpilify
{
  visitor_simpilify(visitable & data):m_data(data){}
  inline void operator()(visitor_simpilify_foo1_var&data)noexcept{m_data.accept(data);number = data.number;};
  inline auto operator()(visitor_simpilify_foo2_var&data)noexcept{m_data.accept(data);number = data.number;};
  inline auto operator()(visitor_simpilify_foo3_var&data)noexcept{m_data.accept(data);number = data.number;};
  inline auto operator()(visitor_simpilify_foo4_var&data)noexcept{m_data.accept(data);number = data.number;};

  visitable &m_data;
  double number;
};


std::vector<std::unique_ptr<foo_base>> data;

void init(){
  if(data.empty()){
    data.reserve(random_data.size());
    for(auto idx : random_data){
      switch(idx){
      case 0:
        data.push_back(std::make_unique<foo1>());
        break;
      case 1:
        data.push_back(std::make_unique<foo2>());
        break;
      case 2:
        data.push_back(std::make_unique<foo3>());
        break;
      case 3:
        data.push_back(std::make_unique<foo4>());
        break;
      }
    }
  }
}

static void init_data(benchmark::State& state) {
  for (auto _ : state) {
    init();
    benchmark::DoNotOptimize(data);
  }
}


static void get_id(benchmark::State& state) {
  init();
  for (auto _ : state) {
    id_type id{0};
    for(auto& ptr : data){
      id += ptr->get_ID();
    }
    benchmark::DoNotOptimize(id);
  }
}

static void get_typeid(benchmark::State& state) {
  init();
  for (auto _ : state) {
    for(auto& ptr : data){
const std::size_t id = std::type_index(typeid(*ptr)).hash_code();
      //std::cout<<id.hash_code()<<std::endl;
    }
  }
}

static void get_id_visitor(benchmark::State& state) {
  init();
  visitor_id visitor;
  for (auto _ : state) {
    id_type id{0};
    for(auto& ptr : data){
      static_cast<visitable&>(*ptr).accept(visitor);
      id += visitor.number;
    }
    benchmark::DoNotOptimize(id);
  }
}

static void get_number(benchmark::State& state) {
  init();
  visitor_imp visitor;
  for (auto _ : state) {
    double number{0};
    for(auto& ptr : data){
      static_cast<visitable&>(*ptr).accept(visitor);
      number += visitor.number;
    }
    benchmark::DoNotOptimize(number);
  }
}

static void double_dispatch(benchmark::State& state) {
  init();
  make_visitor_simpilify visitor;
  for (auto _ : state) {
    double number{0};
    for(auto& ptrA : data){
      static_cast<visitable&>(*ptrA).accept(visitor);
      for(auto& ptrB : data){
        visitor.m_data->apply(static_cast<visitable&>(*ptrB));
        number += visitor.m_data->number;
      }
    }
    benchmark::DoNotOptimize(number);
  }
}

static void double_dispatch_variant(benchmark::State& state) {
  init();
  make_visitor_simpilify_var visitor;
  for (auto _ : state) {
    double number{0};
    for(auto& ptrA : data){
      static_cast<visitable&>(*ptrA).accept(visitor);
      for(auto& ptrB : data){
        visitor_simpilify visitorB(static_cast<visitable&>(*ptrB));
        std::visit(visitorB, visitor.m_data);
        //visitor.m_data->apply(static_cast<visitable&>(*ptrB));
        number += visitorB.number;
      }
    }
    benchmark::DoNotOptimize(number);
  }
}
}//NAMESPACE POLY




namespace var {
struct foo_base{
  foo_base() = default;
  foo_base(foo_base && data):
                                   _rhs(std::move(data._rhs)),
                                   _lhs(std::move(data._lhs)),
                                   _data(std::move(data._data))
  {}

  virtual~foo_base() = default;

  std::unique_ptr<foo_base> _rhs;
  std::unique_ptr<foo_base> _lhs;
  std::vector<std::unique_ptr<foo_base>> _data;
  std::string m_name;
};

struct foo1 final : public foo_base
{
  foo1(){}
  //explicit foo1(foo1 && data):foo_base(std::move(data)){};
  constexpr id_type get_ID() const noexcept {return 0;}
};
struct foo2 final : public foo_base
{
  foo2(){}
  constexpr id_type get_ID() const noexcept {return 1;}
};
struct foo3 final : public foo_base
{
  foo3(){}
  constexpr id_type get_ID() const noexcept {return 2;}
};
struct foo4 final : public foo_base
{
  foo4(){}
  constexpr id_type get_ID() const noexcept {return 3;}
};



using variant = std::variant<foo1,foo2,foo3,foo4>;
std::vector<std::unique_ptr<variant>> data;

void init(){
  if(data.empty()){
    data.reserve(random_data.size());
    for(auto idx : random_data){
      switch(idx){
      case 0:
        data.push_back(std::make_unique<variant>(foo1()));
        break;
      case 1:
        data.push_back(std::make_unique<variant>(foo2()));
        break;
      case 2:
        data.push_back(std::make_unique<variant>(foo3()));
        break;
      case 3:
        data.push_back(std::make_unique<variant>(foo4()));
        break;
      }
    }
  }
}


struct visitor_id
{
  inline auto operator()(foo1 const& foo)noexcept{return foo.get_ID();}
  inline auto operator()(foo2 const& foo)noexcept{return foo.get_ID();}
  inline auto operator()(foo3 const& foo)noexcept{return foo.get_ID();}
  inline auto operator()(foo4 const& foo)noexcept{return foo.get_ID();}
};

struct visitor_number{
  inline auto operator()(foo1 const& foo)noexcept{return rand();}
  inline auto operator()(foo2 const& foo)noexcept{return rand();}
  inline auto operator()(foo3 const& foo)noexcept{return rand();}
  inline auto operator()(foo4 const& foo)noexcept{return rand();}
};

struct visitor_simpilify_foo1// final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo1(foo1 const& data):m_data(data){}
  explicit visitor_simpilify_foo1(visitor_simpilify_foo1 const& data):m_data(data.m_data){}
  inline auto operator()(foo1&)noexcept{return rand();};
  inline auto operator()(foo2&)noexcept{return rand();};
  inline auto operator()(foo3&)noexcept{return rand();};
  inline auto operator()(foo4&)noexcept{return rand();};
  foo1 const& m_data;
};

struct visitor_simpilify_foo2 //final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo2(foo2 const& data):m_data(data){}
  explicit visitor_simpilify_foo2(visitor_simpilify_foo2 const& data):m_data(data.m_data){}
  inline auto operator()(foo1&)noexcept{return rand();};
  inline auto operator()(foo2&)noexcept{return rand();};
  inline auto operator()(foo3&)noexcept{return rand();};
  inline auto operator()(foo4&)noexcept{return rand();};
  foo2 const& m_data;
};

struct visitor_simpilify_foo3// final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo3(foo3 const& data):m_data(data){}
  explicit visitor_simpilify_foo3(visitor_simpilify_foo3 const& data):m_data(data.m_data){}
  inline auto operator()(foo1&)noexcept{return rand();};
  inline auto operator()(foo2&)noexcept{return rand();};
  inline auto operator()(foo3&)noexcept{return rand();};
  inline auto operator()(foo4&)noexcept{return rand();};
  foo3 const& m_data;
};

struct visitor_simpilify_foo4 //final : public visitor_simpilify_base, public visitor
{
  explicit visitor_simpilify_foo4(foo4 const& data):m_data(data){}
  explicit visitor_simpilify_foo4(visitor_simpilify_foo4 const& data):m_data(data.m_data){}
  explicit visitor_simpilify_foo4(visitor_simpilify_foo4 && data):m_data(data.m_data){}
  inline auto operator()(foo1&)noexcept{return rand();};
  inline auto operator()(foo2&)noexcept{return rand();};
  inline auto operator()(foo3&)noexcept{return rand();};
  inline auto operator()(foo4&)noexcept{return rand();};
  foo4 const& m_data;
};


struct make_visitor_simpilify// final : public visitor
{
  using var = std::variant<visitor_simpilify_foo1,visitor_simpilify_foo2,visitor_simpilify_foo3,visitor_simpilify_foo4>;
  inline var operator()(foo1&data)noexcept{return var(std::in_place_index<0>, visitor_simpilify_foo1(data));};
  inline var operator()(foo2&data)noexcept{return var(std::in_place_index<1>, visitor_simpilify_foo2(data));};
  inline var operator()(foo3&data)noexcept{return var(std::in_place_index<2>, visitor_simpilify_foo3(data));};
  inline var operator()(foo4&data)noexcept{return var(std::in_place_index<3>, visitor_simpilify_foo4(data));};
};

struct visitor_simpilify
{
  visitor_simpilify(variant & data):m_data(data){}
  inline auto operator()(visitor_simpilify_foo1&data)noexcept{return std::visit(data,m_data);};
  inline auto operator()(visitor_simpilify_foo2&data)noexcept{return std::visit(data,m_data);};
  inline auto operator()(visitor_simpilify_foo3&data)noexcept{return std::visit(data,m_data);};
  inline auto operator()(visitor_simpilify_foo4&data)noexcept{return std::visit(data,m_data);};

  variant &m_data;
};

static void init_data(benchmark::State& state) {
  for (auto _ : state) {
    init();
    benchmark::DoNotOptimize(data);
  }
}


static void get_id(benchmark::State& state) {
  init();
  for (auto _ : state) {
    id_type id{0};
    for(auto& ptr : data){
      id += std::visit(visitor_id(),*ptr);
    }
    benchmark::DoNotOptimize(id);
  }
}

static void get_number(benchmark::State& state) {
  init();
  for (auto _ : state) {
    double id{0};
    for(auto& ptr : data){
      id += std::visit(visitor_number(),*ptr);
    }
    benchmark::DoNotOptimize(id);
  }
}

auto func = [] (auto const& ref) {
  using type = std::decay_t<decltype(ref)>;
  if constexpr (std::is_same<type, foo1>::value) {
    return rand();
  } else if constexpr (std::is_same<type, foo2>::value) {
    return rand();
  } else if constexpr (std::is_same<type, foo3>::value)  {
    return rand();
  } else if constexpr (std::is_same<type, foo4>::value) {
    return rand();
  } else {
    return 0;
  }
};

static void get_number_constexp(benchmark::State& state) {



  init();
  for (auto _ : state) {
    double id{0};
    for(auto& ptr : data){
      id += std::visit(func,*ptr);
    }
    benchmark::DoNotOptimize(id);
  }
}

static void double_dispatch(benchmark::State& state) {
  init();
  make_visitor_simpilify visitor;
  for (auto _ : state) {
    double number{0};
    for(auto& ptrA : data){
      auto visitorA = std::visit(visitor, *ptrA);
      for(auto& ptrB : data){
        visitor_simpilify visitorB(*ptrB);
        number += std::visit(visitorB, visitorA);
      }
    }
    benchmark::DoNotOptimize(number);
  }
}

}
BENCHMARK(poly::init_data);
BENCHMARK(poly::get_id);
BENCHMARK(poly::get_id_visitor);
BENCHMARK(poly::get_typeid);
BENCHMARK(poly::get_number);
BENCHMARK(poly::double_dispatch);
BENCHMARK(poly::double_dispatch_variant);

BENCHMARK(var::init_data);
BENCHMARK(var::get_id);
BENCHMARK(var::get_number);
BENCHMARK(var::get_number_constexp);
BENCHMARK(var::double_dispatch);

BENCHMARK_MAIN();
