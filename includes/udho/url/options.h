#ifndef UDHO_URL_OPTIONS_H
#define UDHO_URL_OPTIONS_H

#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>

namespace udho{
namespace url{

/**
 * @brief The basic_options encapsulates multiple configurable parameters
 * @ingroup Router
 */
template <typename... Params>
class basic_options: private udho::hazo::map_d<Params...>{
    typedef udho::hazo::map_d<Params...> map_type;

public:
    using map_type::map_type;
    using map_type::operator[];
    enum {

        /**
         * @brief number of parameters
         */
        length = map_type::depth +1
    };

    /**
     * @brief applies the parameters on the input superset
     * @param superset
     * @return number of properties in the superset modified
     */
    template <typename SupersetT>
    std::size_t apply(SupersetT& superset) const {
        std::size_t count = 0;
        const basic_options& self = *this;
        map_type::visit([&superset, &count](auto& p) mutable {
            superset[p.val] = p;
            ++count;
        });
        return count;
    }
};

template <>
struct basic_options<>{
    enum {
        length = 0
    };

    /**
     * @brief applies the parameters on the input superset
     * @param superset
     * @return number of properties in the superset modified
     */
    template <typename SupersetT>
    std::size_t apply(SupersetT& superset) const {
        return 0;
    }
};

/**
 * @brief no options
 * @ingroup Router
 */
using no_options = udho::url::basic_options<>;

/**
 * @brief convenience function to create options for url
 * @param params
 * @ingroup Router
 * @return
 *
 * @code
 * namespace opt {
 *   HAZO_ELEMENT(a, std::string);
 *   HAZO_ELEMENT(b, std::size_t);
 *   HAZO_ELEMENT(c, double);
 *   HAZO_ELEMENT(d, bool);
 *   HAZO_ELEMENT(e, int);
 *}
 * auto options = udho::url::options(opt::a("a"), opt::b(42), opt::c(4.2));
 * @endcode
 */
template <typename... Params>
basic_options<Params...> options(Params&&... params){
    return basic_options<Params...>{std::forward<Params>(params)...};
}

}
}

#endif // UDHO_URL_OPTIONS_H
