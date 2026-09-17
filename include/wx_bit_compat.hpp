#pragma once
#include <bit>
#if defined(WX_XBOX) && !defined(__cpp_lib_bit_cast)
#include <type_traits>
// Supply the missing C++20 API in the pinned nxdk libc++ implementation.
namespace std {
template<class To,class From>
constexpr To bit_cast(const From& value) noexcept {
    static_assert(sizeof(To)==sizeof(From),"bit_cast sizes must match");
    static_assert(is_trivially_copyable<To>::value&&is_trivially_copyable<From>::value,"bit_cast requires trivial types");
    return __builtin_bit_cast(To,value);
}
}
#endif
