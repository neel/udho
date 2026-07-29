#ifndef UDHO_VIEW_RESOURCES_ASSET_INFO_H
#define UDHO_VIEW_RESOURCES_ASSET_INFO_H

#include <string>
#include <memory>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/asset/utils.h>
#include <udho/view/data/data.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{

/**
 * @addtogroup DoxyG_view_resources_assets
 * @{
 */

/**
 * @brief description of an asset
 */
class asset_registration_info{
    using resource_ptr = std::unique_ptr<asset::abstract_resource>;

    std::string  _prefix;
    resource_ptr _res;

  public:
    asset_registration_info() = delete;
    inline asset_registration_info(const std::string& prefix, resource_ptr&& res): _prefix(prefix), _res(std::move(res)) {
        assert(prefix.front() != '/' && prefix.back() != '/');
    }
    asset_registration_info(const asset_registration_info&) = delete;
    inline asset_registration_info(asset_registration_info&& other): _prefix(std::move(other._prefix)), _res(std::move(other._res)) {}
    asset_registration_info& operator=(const asset_registration_info&) = delete;
    inline asset_registration_info& operator=(asset_registration_info&& other) noexcept {
        _prefix = std::move(other._prefix);
        _res = std::move(other._res);
        return *this;
    }
  public:
    /**
     * @brief Name of the view
     */
    inline const std::string& name() const { return _res->name(); }
    /**
     * @brief The prefix used in resource identification.
     */
    inline const std::string& prefix() const { return _prefix; }
    inline asset::source::type source() const { return _res->source(); }
    inline bool owned() const { return _res->owned(); }

    inline basic_resource<asset::type::css>& css() { return cast<asset::type::css>(); }
    inline basic_resource<asset::type::js>&  js()  { return cast<asset::type::js> (); }
    inline basic_resource<asset::type::txt>& txt() { return cast<asset::type::txt>(); }
    inline basic_resource<asset::type::img>& img() { return cast<asset::type::img>(); }

    inline std::string mime() const { return _res->mime(); }
    template <typename OstreamT>
    inline std::size_t write(OstreamT& ostream) const{ return _res->write(ostream); }
    template <typename OstreamT>
    inline std::size_t write_contents(OstreamT& ostream) const{ return _res->write_contents(ostream); }
    /**
     * @brief type of the asset
     */
    inline asset::type type() const { return _res->type(); }
    inline std::string type_str() const { return udho::view::resources::asset::utils::to_string(_res->type()); }

    template <asset::type AssetType>
    const udho::view::resources::asset::basic_resource<AssetType>& cast() const {
        return dynamic_cast<const udho::view::resources::asset::basic_resource<AssetType>&>(*_res);
    }
    template <asset::type AssetType>
    udho::view::resources::asset::basic_resource<AssetType>& cast() {
        return dynamic_cast<udho::view::resources::asset::basic_resource<AssetType>&>(*_res);
    }

    inline bool less(const asset_registration_info& other) const {
        if(type() == other.type()){
            if(prefix() == other.prefix()){
                return name() < other.name();
            }
            return prefix() < other.prefix();
        }
        return type() < other.type();
    }

    friend auto metatype(udho::view::data::type<asset_registration_info>){
        using namespace udho::view::data;

        return assoc("asset_description"),
                fvar("name",        &asset_registration_info::name),
                fvar("prefix",      &asset_registration_info::prefix),
                fvar("owned",       &asset_registration_info::owned),
                fvar("mime",        &asset_registration_info::mime),
                fvar("type",        &asset_registration_info::type_str);
    }
};

/** @} */

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_ASSET_INFO_H
