#ifndef UDHO_VIEW_RESOURCES_ASSET_UTILS_H
#define UDHO_VIEW_RESOURCES_ASSET_UTILS_H

#include <string>
#include <udho/view/resources/fwd.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{

/**
 * @addtogroup DoxyG_view_resources_assets
 * @{
 */

namespace utils{

inline std::string to_string(asset::type type){
    std::string asset_type_str = "unkown";
    switch(type){
    case type::css:
        asset_type_str = "css";
        break;
    case type::js:
        asset_type_str = "js";
        break;
    case type::txt:
        asset_type_str = "txt";
        break;
    case type::img:
        asset_type_str = "img";
        break;
    case type::none:
        asset_type_str = "none";
        break;
    }
    return asset_type_str;
}

inline std::string to_string(asset::source::type type){
    std::string asset_source_type_str = "unkown";
    switch(type){
    case source::type::memory:
        asset_source_type_str = "memory";
        break;
    case source::type::disk:
        asset_source_type_str = "disk";
        break;
    case source::type::remote:
        asset_source_type_str = "remote";
        break;
    case source::type::none:
        asset_source_type_str = "none";
        break;
    }
    return asset_source_type_str;
}


}

/** @} */

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_ASSET_UTILS_H
