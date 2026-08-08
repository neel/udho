#ifndef UDHO_PAGES_REPR_H
#define UDHO_PAGES_REPR_H

#include <string>
#include <udho/view/tmpl/layout/repr.h>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/pages/data.h>
#include <udho/url/summary.h>
#include <udho/utils/format.h>
#include <udho/utils/encoding.h>
#include <udho/utils/string_view.h>
#include <boost/algorithm/string/case_conv.hpp>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

template<>
struct repr<udho::pages::system::data::listing> {
    using loader_js  = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css>;
    using data_type  = udho::pages::system::data::listing;

    repr(const data_type& data): _data(data) {}

    void include(loader_css& loader) const {
        loader.add("udho", "system.css");
        loader.add("udho", "listing.css");
    }

    void include(loader_js&) const {}

    template<typename ContextT>
    std::string operator()(const ContextT& ctx) const {
        (void)ctx;

        std::string html;
        html.append(html_begin.data(), html_begin.size());

        for(const auto& entry: _data) {
            const auto extension   = entry.extension();
            const auto entry_class = entry.is_directory() ? "udho-icon-directory" : "udho-icon-file";
            const auto ext_class   = extension.empty() ? "" : "udho-ext-" + boost::algorithm::to_lower_copy(extension);
            const auto ext_label   = extension.empty() ? "-" : boost::algorithm::to_upper_copy(extension);

            html += udho::utils::format(
                html_entry,
                udho::utils::encode::escape(_data.label()),
                udho::utils::encode::escape(entry.url()),
                entry_class,
                udho::utils::encode::escape(entry.name()),
                udho::utils::encode::escape(entry.type()),
                ext_class,
                udho::utils::encode::escape(ext_label),
                udho::utils::encode::escape(entry.mime()),
                udho::utils::encode::escape(entry.size())
            );
        }

        html.append(html_end.data(), html_end.size());
        return html;
    }

private:
    static constexpr udho::utils::string_view html_begin = R"HTML(
<div class="listing-container">
    <div class="udho-container">
        <table class="udho-listing">
            <thead>
                <tr>
                    <th scope="col">Name</th>
                    <th scope="col">Type</th>
                    <th scope="col">Ext</th>
                    <th scope="col">Mime</th>
                    <th scope="col">Size</th>
                </tr>
            </thead>
            <tbody>
)HTML";

    static constexpr udho::utils::string_view html_entry = R"HTML(
                <tr>
                    <td>
                        <a class="udho-listing-item-{}" href="{}">
                            <span class="{}"></span>
                            {}
                        </a>
                    </td>
                    <td><code>{}</code></td>
                    <td class="{}">{}</td>
                    <td><code>{}</code></td>
                    <td>{}</td>
                </tr>
)HTML";

    static constexpr udho::utils::string_view html_end = R"HTML(
            </tbody>
        </table>
    </div>
</div>
)HTML";

    const data_type& _data;
};


template<>
struct repr<udho::pages::system::data::listings> {
    using loader_js  = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css>;
    using data_type  = udho::pages::system::data::listings;

    repr(const data_type& data): _data(data) {}

    void include(loader_css& loader) const {
        loader.add("udho", "system.css");
        loader.add("udho", "listing.css");
    }

    void include(loader_js& loader) const {
        loader.add("udho", "tabs.js");
    }

    template<typename ContextT>
    std::string operator()(const ContextT& ctx) const {
        std::string html;
        html.append(html_begin.data(), html_begin.size());

        std::size_t index = 0;
        for(const auto& listing: _data) {
            html += udho::utils::format(
                html_button,
                index++ == 0 ? "active-tab" : "",
                udho::utils::encode::escape(listing.label()),
                udho::utils::encode::escape(listing.label())
            );
        }

        html += udho::utils::format(
            html_nav,
            udho::utils::encode::escape(_data.current()),
            udho::utils::encode::escape(_data.parent())
        );

        index = 0;
        for(const auto& listing: _data) {
            html += udho::utils::format(
                html_content,
                index++ == 0 ? "active-content" : "",
                udho::utils::encode::escape(listing.label()),
                udho::utils::encode::escape(listing.label()),
                repr<udho::pages::system::data::listing>{listing}(ctx)
            );
        }

        html.append(html_end.data(), html_end.size());
        return html;
    }

private:
    static constexpr udho::utils::string_view html_begin = R"HTML(
<div class="tab-container">
    <div class="tab-buttons">
)HTML";

    static constexpr udho::utils::string_view html_button =
        R"HTML(<button class="tab-btn {}" data-target="{}">{}</button>)HTML";

    static constexpr udho::utils::string_view html_nav = R"HTML(
        <div class="nav-buttons">
            <span class="nav-current">Index of {} </span>
            <a class="nav-up" href="{}">&nbsp;</a>
        </div>
    </div>
)HTML";

    static constexpr udho::utils::string_view html_content = R"HTML(
    <div class="tab-content {}" id="{}">
        <h3 class="listing-heading"> {} </h3>
        {}
    </div>
)HTML";

    static constexpr udho::utils::string_view html_end = R"HTML(
</div>
)HTML";

    const data_type& _data;
};


template<>
struct repr<udho::url::summary::router> {
    using data_type  = udho::url::summary::router;
    using loader_js  = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css>;

    repr(const data_type& data): _data(data) {}

    void include(loader_css& loader) const {
        loader.add("udho", "system.css");
        loader.add("udho", "routes.css");
    }

    void include(loader_js&) const {}

    template<typename ContextT>
    std::string operator()(const ContextT& ctx) const {
        (void)ctx;

        std::string html;
        html.append(html_begin.data(), html_begin.size());

        for(const auto& [mount_label, mount]: _data) {
            html += udho::utils::format(
                html_mount_begin,
                udho::utils::encode::escape(mount_label),
                udho::utils::encode::escape(mount.path())
            );

            for(const auto& [route_label, route]: mount) {
                (void)route_label;
                const auto method_class = boost::algorithm::to_lower_copy(route.match().method());

                html += udho::utils::format(
                    html_route,
                    method_class,
                    udho::utils::encode::escape(route.match().method()),
                    udho::utils::encode::escape(route.match().format()),
                    udho::utils::encode::escape(route.match().pattern()),
                    udho::utils::encode::escape(route.match().replacement()),
                    udho::utils::encode::escape(route.slot().key()),
                    route.slot().nargs() - 1,
                    udho::utils::encode::escape(route.slot().symbol())
                );
            }

            html.append(html_mount_end.data(), html_mount_end.size());
        }

        html.append(html_end.data(), html_end.size());
        return html;
    }

private:
    static constexpr udho::utils::string_view html_begin = R"HTML(
<div class="routes-container">
)HTML";

    static constexpr udho::utils::string_view html_mount_begin = R"HTML(
    <!-- begin udho::mark::mount -->
    <div class="mount-point">
        <!-- begin udho::mark::point -->
        <div class="mount-header">
            <span class="mount-label">{}</span>
            <span class="mount-path">{}</span>
        </div>
        <!-- end udho::mark::point -->

        <!-- begin udho::mark::table -->
        <div class="routes-table">
            <div class="table-header">
                <span>Method</span>
                <span>Pattern</span>
                <span>Replacement</span>
                <span>Label</span>
                <span>Callback</span>
            </div>
)HTML";

    static constexpr udho::utils::string_view html_route = R"HTML(
            <!-- begin udho::mark::route -->
            <div class="route-entry">
                <span class="http-method {}" data-label="Method">{}</span>
                <div class="route-pattern" data-label="Pattern">
                    <span class="route-pattern-format" data-label="Format">{}</span>
                    <span class="route-pattern-str" data-label="Pattern">{}</span>
                </div>
                <code class="route-replacement" data-label="Replacement">{}</code>
                <div class="route-label" data-label="Label">
                    <span class="route-label-name">{}</span>
                    <span class="route-label-args">{}</span>
                </div>
                <code class="route-callback" data-label="Callback"><pre>{}</pre></code>
            </div>
            <!-- end udho::mark::route -->
)HTML";

    static constexpr udho::utils::string_view html_mount_end = R"HTML(
        </div>
        <!-- end udho::mark::table -->
    </div>
    <!-- end udho::mark::mount -->
)HTML";

    static constexpr udho::utils::string_view html_end = R"HTML(
</div>
)HTML";

    const data_type& _data;
};


template<>
struct repr<udho::pages::system::data::listing_header> {
    using loader_js  = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css>;
    using data_type  = udho::pages::system::data::listing_header;

    repr(const data_type& data): _data(data) {}

    void include(loader_css& loader) const {
        loader.add("udho", "header.css");
    }

    void include(loader_js&) const {}

    template<typename ContextT>
    std::string operator()(const ContextT& ctx) const {
        return udho::utils::format(
            html_content,
            _data.code(),
            udho::utils::encode::escape(_data.message()),
            udho::utils::encode::escape(ctx.portal().resources().img().get("udho", "beral.gif").url())
        );
    }

private:
    static constexpr udho::utils::string_view html_content = R"HTML(
<div class="logo">
    <div class="logo-parts http-status">
        <div class="http-status-code">{}</div>
        <div class="http-status-msg">{}</div>
    </div>
    <img class="logo-parts beral-logo" src="{}" />
</div>
)HTML";

    const data_type& _data;
};


template<>
struct repr<udho::pages::system::data::status_info> {
    using loader_js  = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css>;
    using data_type  = udho::pages::system::data::status_info;

    repr(const data_type& data): _data(data) {}

    void include(loader_css&) const {}
    void include(loader_js&) const {}

    template<typename ContextT>
    std::string operator()(const ContextT& ctx) const {
        return udho::utils::format(
            html_content,
            udho::utils::encode::escape(_data.os),
            udho::utils::encode::escape(_data.cpp),
            udho::utils::encode::escape(_data.compiler),
            udho::utils::encode::escape(_data.boost),
            udho::utils::encode::escape("None"),
            udho::utils::encode::escape(_data.memory),
            udho::utils::encode::escape(_data.time)
        );
    }

private:
    static constexpr udho::utils::string_view html_content = R"HTML(
<ul class="udho-deploy-info">
    <li> udho </li>
    <li> {} </li>
    <li> {} </li>
    <li> {} </li>
    <li> {} </li>
    <li> {} </li>
    <li> {} </li>
    <li> {} </li>
</ul>
)HTML";

    const data_type& _data;
};

}
}
}
}

#endif // UDHO_PAGES_REPR_H