#ifndef UDHO_VIEW_RESOURCES_ASSET_PROXY_H
#define UDHO_VIEW_RESOURCES_ASSET_PROXY_H

#include <string>
#include <udho/view/resources/asset/info.h>
#include <udho/view/data/data.h>
#include <udho/url/utils.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{

/**
 * @struct proxy
 * @ingroup view
 * @brief proxies an asset.
 */
struct proxy{

    /**
     * @brief Constructs a proxy for a given resource and bridge.
     * @param name The name of the resource
     * @param prefix The prefix used in resource identification.
     * @param bridge Reference to the bridge used for resource execution.
     */
    inline proxy(const asset_registration_info& desc,  const std::string& base): _desc(desc), _base(base) {}

    /**
     * @brief Returns the name of the resource associated with this proxy.
     * @return The name of the resource.
     */
    inline std::string name() const { return _desc.name(); }

    /**
     * @brief Returns the prefix of the resource associated with this proxy.
     * @return The prefix of the resource.
     */
    inline std::string prefix() const { return _desc.prefix(); }

    /**
     * @brief Returns the prefix of the resource associated with this proxy.
     * @return The prefix of the resource.
     */
    inline std::string mime() const { return _desc.mime(); }

    /**
     * @brief type of the asset
     */
    inline asset::type type() const { return _desc.type(); }

    /**
     * @brief gets base url of the asset store
     * @details an asset with prefix "blog", name "theme.css" will be served as /base/blog/theme.css
     */
    const std::string& base() const { return _base; }

    /**
     * @brief expected url of the asset
     */
    const std::string url() const {
        return udho::url::format("{}{}/{}", udho::url::utils::slash_quote(_base), prefix(), name());
    }

    /**
     * @brief write the asset contents to stream
     */
    template <typename OstreamT>
    std::size_t write(OstreamT& ostream) const{
        return _desc.write(ostream);
    }

    template <typename OstreamT>
    std::size_t write_contents(OstreamT& ostream) const{
        return _desc.write_contents(ostream);
    }

    template <asset::type AssetType>
    const udho::view::resources::asset::basic_resource<AssetType>& cast() const {
        return _desc.cast<AssetType>();
    }

    /**
     * @brief exposed to lua via the following properties
     * +----------+------------+
     * | name     | property   |
     * | prefix   | property   |
     * | url      | property   |
     * +----------+------------+
     */
    friend auto metatype(udho::view::data::type<proxy>){
        using namespace udho::view::data;

        return assoc("resources_asset_proxy"),
               fvar("name",   &proxy::name),
               fvar("type",   &proxy::type),
               fvar("prefix", &proxy::prefix),
               fvar("mime",   &proxy::mime),
               fvar("url",    &proxy::url);
    }

    bool less(const proxy& other) const {
        return _desc.less(other._desc);
    }

private:
    const asset_registration_info& _desc;
    const std::string& _base;
};

inline bool operator<(const proxy& l, const proxy& r) {
    return l.less(r);
}
inline bool operator==(const proxy& l, const proxy& r) {
    return l.url() == r.url();
}

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_ASSET_PROXY_H
