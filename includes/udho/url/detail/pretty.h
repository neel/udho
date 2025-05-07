#ifndef UDHO_URL_DETAIL_PRETTY_H
#define UDHO_URL_DETAIL_PRETTY_H

#include <string>
#include <regex>

#if defined(__clang__) || defined(__GNUC__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNC_SIG __FUNCSIG__
#endif

namespace udho{
namespace url{

namespace detail{

template <typename T>
struct type_name_generator {
    static std::string get_expanded() {
#if defined(__clang__) || defined(__GNUC__)
        constexpr auto pretty_function = __PRETTY_FUNCTION__;
        constexpr auto prefix_len = sizeof("static std::string udho::url::detail::type_name_generator<T>::get_expanded() [with T = ") - 1;
#elif defined(_MSC_VER)
        constexpr auto pretty_function = __FUNCSIG__;
        constexpr auto prefix_len = sizeof("struct std::string __cdecl type_name_generator<T>::get_expanded(void) [T = ") - 1;
#endif
        constexpr auto suffix_len = sizeof("]") - 1;

        std::string result(pretty_function);
        result = result.substr(prefix_len, result.size() - prefix_len - suffix_len);

        result = std::regex_replace(result, std::regex("(class |struct |enum |union |\\s)"), "");
        return result;
    }
};

class symbol_cleaner {
    using substitution_pair = std::pair<std::string, std::string>;

    static const std::vector<substitution_pair>& substitutions() {
        static const auto subs = []{
            std::vector<substitution_pair> replacements;

            #define ADD_SUBSTITUTION_HELPER(Type, Simple) replacements.emplace_back(type_name_generator<Type>::get_expanded(), Simple)
            ADD_SUBSTITUTION_HELPER(std::string, "std::string");
            ADD_SUBSTITUTION_HELPER(std::string_view, "std::string_view");
            #undef ADD_SUBSTITUTION_HELPER

            std::sort(replacements.begin(), replacements.end(), [](const auto& a, const auto& b) { return a.first.length() > b.first.length(); });
            return replacements;
        }();

        return subs;
    }

public:
    static std::string clean(std::string symbol) {
        // Apply type substitutions
        const std::vector<substitution_pair>& subs = substitutions();
        for (const auto& [expanded, simple] : subs) {
            size_t pos = 0;
            while ((pos = symbol.find(expanded, pos)) != std::string::npos) {
                symbol.replace(pos, expanded.length(), simple);
                pos += simple.length();
            }
        }

        // General namespace cleanup
        const std::vector<substitution_pair> regex_rules = {
            {"std::__cxx11::", "std::"},
            {"std::__1::", "std::"},
            {"__gnu_cxx::", "std::"},
            {",std::allocator<[^>]+>", ""},
            {" ,", ","},
            {" >", ">"}
        };

        for (const auto& [pattern, replacement] : regex_rules) {
            symbol = std::regex_replace(symbol, std::regex(pattern), replacement);
        }

        return symbol;
    }
};


}

}
}

#endif // UDHO_URL_DETAIL_PRETTY_H
