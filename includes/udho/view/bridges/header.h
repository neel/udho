#ifndef UDHO_VIEW_DATA_BRIDGES_HEADER_H
#define UDHO_VIEW_DATA_BRIDGES_HEADER_H

#include <set>
#include <map>
#include <string>
#include <udho/view/resources/fwd.h>
#include <udho/view/data/associative.h>
#include <udho/view/data/operators.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @brief meta block configuration object
 * @details A destription object is passed to the meta block, which is modified by the instruction present in that block.
 *          After that the description object is accessed to interpret the consfigurations expressed by the view template.
 */
struct view_header{
    struct vars_{
        std::string data    = "d";
        std::string context = "ctx";

        friend auto metatype(udho::view::data::type<vars_>){
            using namespace udho::view::data;

            return assoc("vars_"),
                mvar("data",    &vars_::data),
                mvar("context", &vars_::context);
        }
    };

    struct includes_{
        struct resource_info_{
            std::string prefix;
            std::string name;

            bool operator<(const resource_info_& other) const {
                return std::tie(prefix, name) < std::tie(other.prefix, other.name);
            }

            bool operator==(const resource_info_& other) const noexcept {
                return std::tie(prefix, name) == std::tie(other.prefix, other.prefix);
            }
        };

        using asset_type     = udho::view::resources::asset::type;
        using resources_set  = std::set<resource_info_>;
        using const_iterator = typename resources_set::const_iterator;
        using includes_map   = std::map<asset_type, resources_set>;

        /**
         * Checks if the resource is already added. if added does not add again.
         */
        bool add(asset_type type, const std::string& prefix, const std::string& name){
            auto mit = _includes.find(type);
            if(mit == _includes.end()){
                mit = _includes.insert(std::make_pair(type, std::set<resource_info_>{})).first;
            }

            auto sit = mit->second.insert(resource_info_{prefix, name});
            return sit.second;
        }

        boost::iterator_range<const_iterator> resources(asset_type type) const noexcept {
            if(auto it = _includes.find(type); it != _includes.end()) {
                return boost::make_iterator_range(it->second);
            }
            return {};
        }

        void add_js(const std::string& prefix, const std::string& name)  { add(asset_type::js, prefix, name); }
        void add_css(const std::string& prefix, const std::string& name) { add(asset_type::css, prefix, name); }

        auto js() const { return resources(asset_type::js); }
        auto css() const { return resources(asset_type::css); }

        friend auto metatype(udho::view::data::type<includes_>){
            using namespace udho::view::data;

            return assoc("includes_"),
                func("js",  &includes_::add_js),
                func("css", &includes_::add_css);
        }

        includes_map _includes;
    };

    std::string name;
    std::string bridge;
    vars_       vars;
    includes_   includes;
    includes_   embeds;
    bool        whitespace = false;

    friend auto metatype(udho::view::data::type<view_header>){
        using namespace udho::view::data;

        return assoc("view_header"),
            mvar("name",       &view_header::name),
            mvar("bridge",     &view_header::bridge),
            mvar("vars",       &view_header::vars),
            mvar("include",    &view_header::includes),
            mvar("embed",      &view_header::embeds),
            mvar("whitespace", &view_header::whitespace);
    }
};

}
}
}
}

#endif // UDHO_VIEW_DATA_BRIDGES_HEADER_H
