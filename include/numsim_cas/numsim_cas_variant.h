#ifndef NUMSIM_CAS_VARIANT_H
#define NUMSIM_CAS_VARIANT_H

#define NUMSIM_CAS_USE_STD_VARIANT

#if defined(NUMSIM_CAS_USE_STD_VARIANT) &&                                     \
    !defined(NUMSIM_CAS_USE_BOOST_VARIANT)
#include <variant>

namespace numsim::cas {
template <typename... Args> using variant = std::variant<Args...>;
using std::visit;
} // namespace numsim::cas

#elif !defined(NUMSIM_CAS_USE_STD_VARIANT) &&                                  \
    defined(NUMSIM_CAS_USE_BOOST_VARIANT)
#include <boost/variant2>

namespace numsim::cas {
template <typename... Args> using variant = boost::variant2::variant<Args...>;
using boost::variant2::visit;
} // namespace numsim::cas
#else

#endif

#endif // NUMSIM_CAS_VARIANT_H
