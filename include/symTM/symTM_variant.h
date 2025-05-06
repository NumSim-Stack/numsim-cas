#ifndef SYMTM_VARIANT_H
#define SYMTM_VARIANT_H

#define SYMTM_USE_STD_VARIANT

#if defined(SYMTM_USE_STD_VARIANT) && !defined(SYMTM_USE_BOOST_VARIANT)
#include <variant>

namespace numsim::cas {
template<typename ...Args>
using variant = std::variant<Args...>;
using std::visit;
}

#elif !defined(SYMTM_USE_STD_VARIANT) && defined(SYMTM_USE_BOOST_VARIANT)
#include <boost/variant2>

namespace numsim::cas {
template<typename ...Args>
using variant = boost::variant2::variant<Args...>;
using boost::variant2::visit;
}
#else

#endif

#endif // SYMTM_VARIANT_H
