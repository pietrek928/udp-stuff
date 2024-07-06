#pragma once

#include <stdexcept>
#include <string>
#include <string.h>

namespace std {
    string to_string(const char *s);
    string to_string(string &s);
}
template<const char *sep>
std::string __dbg_args_to_string__() {
    return "";
}
template<const char *sep, class Tfirst, class ... Targs>
std::string __dbg_args_to_string__(Tfirst &first_arg, Targs&... args) {
    return std::to_string(first_arg) + sep + __dbg_args_to_string__<sep>(args...);
}
const char __colon_sep__[] = ":";
const char __space_sep__[] = " ";

#define ASSERT(cond, ...) {if (unlikely(!(cond))) { \
    auto line_num = __LINE__; \
    auto loc_str = __dbg_args_to_string__<__colon_sep__>(__FILE__, __FUNCTION__, line_num); \
    auto args_str = __dbg_args_to_string__<__space_sep__>(__VA_ARGS__); \
    throw std::runtime_error(__dbg_args_to_string__<__space_sep__>(loc_str, "Assert", "`" #cond "`", "failed:", args_str)); \
}}
#define ASSERT_EXC(cond, exc, ...) {if (unlikely(!(cond))) { \
    throw exc(__dbg_args_to_string__<__space_sep__>(__VA_ARGS__)); \
}}
#define ASSERT_EXC_VOID(cond, exc) {if (unlikely(!(cond))) { \
    throw exc(); \
}}

#ifdef DEBUG
#define ASSERT_DBG(...) ASSERT(__VA_ARGS__)
#else /* DEBUG */
#define ASSERT_DBG(...) {}
#endif /* DEBUG */
