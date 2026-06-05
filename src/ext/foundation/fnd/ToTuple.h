#pragma once

#include <cstdlib>
#include <tuple>

#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

namespace HomeCompa::Util
{

#ifdef TO_TUPLE_OVERRIDE_MAX_STRUCT_MEMBERS
	#define MAX_STRUCT_MEMBERS TO_TUPLE_OVERRIDE_MAX_STRUCT_MEMBERS
#else
	#define MAX_STRUCT_MEMBERS 48
#endif

#define SELFINCR(z, n, r) BOOST_PP_COMMA_IF(n) r##n
#define TYPECELL(z, n, r) BOOST_PP_COMMA_IF(n) decltype(r##n) &
#define DCREPEAT(u, n, m, r) BOOST_PP_REPEAT(BOOST_PP_SUB(u, n), m, r)

#define BRANCH(z, n, _)                                                                                                    \
	template <>                                                                                                            \
	struct to_tuple_ref_t<BOOST_PP_SUB(MAX_STRUCT_MEMBERS, n)>                                                             \
	{                                                                                                                      \
		template <class S>                                                                                                 \
		static constexpr auto convert(S &s)                                                                                \
		{                                                                                                                  \
			auto &[DCREPEAT(MAX_STRUCT_MEMBERS, n, SELFINCR, m)] = s;                                                      \
			return std::tuple<DCREPEAT(MAX_STRUCT_MEMBERS, n, TYPECELL, m)>(DCREPEAT(MAX_STRUCT_MEMBERS, n, SELFINCR, m)); \
		}                                                                                                                  \
	};

namespace ToTupleDetails
{

template <std::size_t N>
struct to_tuple_ref_t;

BOOST_PP_REPEAT(MAX_STRUCT_MEMBERS, BRANCH, _)

/* N : [1, MAX_STRUCT_MEMBERS]
* template<>
* struct to_tuple_ref_t<N>
* {
*     template <class S>
*     static constexpr auto convert(S& s)
*     {
*         auto& [m0, m1, ..., mN] = s;
*         return std::tuple<decltype(m0)&, decltype(m1)&, ..., decltype(mN)&>(m0, m1, ..., mN);
*     }
* };
*/

struct UniversalType
{
	template <typename T>
	operator T()
	{
	}
};

template <typename T>
consteval std::size_t MemberCounter(auto... Members)
{
	if constexpr (requires { T { Members... }; } == false)
		return sizeof...(Members) - 1;
	else
		return MemberCounter<T>(Members..., UniversalType {});
}

} // namespace ToTupleDetails

template <class S>
constexpr auto ToTupleRef(S& s)
{
	return ToTupleDetails::to_tuple_ref_t<ToTupleDetails::MemberCounter<S>()>::convert(s);
}

#undef SELFINCR
#undef TYPECELL
#undef DCREPEAT

} // namespace HomeCompa::Util
