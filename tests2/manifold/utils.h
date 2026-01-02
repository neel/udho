#ifndef UDHO_TEST2_MANIFOLD_UTILS_H
#define UDHO_TEST2_MANIFOLD_UTILS_H

#include <cstdlib>
#include <array>
#include <string_view>

namespace udho{
namespace test{

namespace detail{

constexpr std::size_t digits10(std::size_t n) {
    std::size_t d = 1;
    while (n >= 10) { n /= 10; ++d; }
    return d;
}

template<std::size_t N>
struct to_cstring {
    static_assert(N >= 0, "N must be non-negative");

    inline static constexpr auto data = []{
        constexpr std::size_t len = (N == 0) ? 1 : digits10(N);
        std::array<char, len + 1> a{};   // +1 for '\0'

        std::size_t x = N;
        if constexpr (N == 0) {
            a[0] = '0';
        } else {
            for (std::size_t i = 0; i < len; ++i) {
                a[len - 1 - i] = static_cast<char>('0' + (x % 10));
                x /= 10;
            }
        }
        a[len] = '\0';
        return a;
    }();

    inline static constexpr std::string_view view{ data.data(), data.size() - 1 };
};

}

template <std::size_t N>
const char* to_string() {
    return detail::to_cstring<N>::get().data();
}

}
}

#endif // UDHO_TEST2_MANIFOLD_UTILS_H
