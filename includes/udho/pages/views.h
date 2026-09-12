#ifndef UDHO_PAGES_VIEWS_H
#define UDHO_PAGES_VIEWS_H

#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <udho/view/resources/lua.h>

namespace udho{
namespace pages{
namespace system{
namespace views{

constexpr static char template_listing_table[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.css("udho", "system.css") include.css("udho", "listing.css") ?>

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
            <? for i, e in d:ipairs() do ?>
                <tr>
                    <?
                        local entry_class = e.is_dir and 'udho-icon-directory' or 'udho-icon-file'
                        local ext_class = e.extension and 'udho-ext-' .. e.extension:lower() or ''
                    ?>
                    <td>
                        <a class="udho-listing-item-<?= d.label ?>" href="<?= e.url ?>">
                            <span class="<?= entry_class ?>"></span>
                            <?= e.name ?>
                        </a>
                    </td>
                    <td><code><?= e.type ?></code></td>
                    <td class="<?= ext_class ?>"><?= e.extension and string.upper(e.extension) or '-' ?></td>
                    <td><code><?= e.mime ?></code></td>
                    <td><?= e.size ?></td>
                </tr>
            <? end ?>
            </tbody>
        </table>
    </div>
</div>

)TEMPLATE";

constexpr static char template_listing_page[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.css("udho", "system.css") include.css("udho", "listing.css") embed.js("udho", "tabs.js") ?>

<div class='tab-container'>
    <div class='tab-buttons'>
        <? for i, e in d:ipairs() do ?>
            <button class='tab-btn <?= (i == 1) and "active-tab" or "" ?>' data-target='<?= e.label ?>'><?= e.label ?></button>
        <? end ?>
        <div class="nav-buttons">
            <span class="nav-current">Index of <?= d.current ?> </span>
            <a class="nav-up" href="<?= d.parent ?>">&nbsp;</a>
        </div>
    </div>

    <? for i, e in d:ipairs() do ?>
        <div class='tab-content <?= (i == 1) and "active-content" or "" ?>' id='<?= e.label ?>'>
            <h3 class="listing-heading"> <?= e.label ?> </h3>
            <?= ctx:view("udho", "listing_table"):render(e) ?>
        </div>
    <? end ?>
</div>

)TEMPLATE";

constexpr static char template_routes_page[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.css("udho", "system.css") include.css("udho", "routes.css") ?>

<div class='routes-container'>
    <? for k, m in d:pairs() do ?>
        <!-- begin udho::mark::mount -->
        <div class="mount-point">
            <!-- begin udho::mark::point -->
            <div class="mount-header">
                <span class="mount-label"><?= k ?></span>
                <span class="mount-path"><?= m.path ?></span>
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

                <? for l, r in m:pairs() do ?>
                    <!-- begin udho::mark::route -->
                    <div class="route-entry">
                        <span class="http-method <?= r.match.method:lower() ?>" data-label="Method"><?= r.match.method ?></span>
                        <div class="route-pattern" data-label="Pattern">
                            <span class="route-pattern-format" data-label="Format"><?= r.match.format ?></span>
                            <span class="route-pattern-str" data-label="Pattern"><?= r.match.pattern ?></span>
                        </div>
                        <code class="route-replacement" data-label="Replacement"><?= r.match.replacement ?></code>

                        <div class="route-label" data-label="Label">
                            <span class="route-label-name"><?= r.slot.key ?></span>
                            <span class="route-label-args"><?= r.slot.nargs -1 ?></span>
                        </div>
                        <code class="route-callback" data-label="Callback"><pre><?= udho.utils.htmlescape(r.slot.symbol) ?></pre></code>
                    </div>
                    <!-- end udho::mark::route -->
                <? end ?>
            </div>
            <!-- end udho::mark::table -->
        </div>
        <!-- end udho::mark::mount -->
    <? end ?>
</div>

)TEMPLATE";

constexpr static char template_listing_header[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.css("udho", "header.css") ?>

<div class="logo">
    <div class="logo-parts http-status">
        <div class="http-status-code">
            <?= d.code ?>
        </div>
        <div class="http-status-msg">
            <?= d.message ?>
        </div>
    </div>
    <img class="logo-parts beral-logo" src="<?= ctx.portal.resources.img:get('udho', 'beral.gif').url ?>" />
</div>
)TEMPLATE";

constexpr static char template_listing_status[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
<ul class="udho-deploy-info">
    <li> udho </li>
    <li> <?= d.os ?> </li>
    <li> <?= d.cpp ?> </li>
    <li> <?= d.compiler ?> </li>
    <li> <?= d.boost ?> </li>
    <li> <?= ctx.portal.resources.bridges_label ?> </li>
    <li> <?= d.memory ?> </li>
    <li> <?= d.time ?> </li>
</ul>
)TEMPLATE";

/**
 * @brief Checks whether all views required by the system pages are registered.
 * @tparam Bridges Bridge types supported by the store.
 * @param store Resource store to inspect.
 * @return True for stores without bridges, or when every system Lua view is present.
 * @ingroup DoxyG_pages
 */
template <typename... Bridges>
bool ready(udho::view::resources::store<Bridges...>& store){
    using store_type = udho::view::resources::store<Bridges...>;

    if constexpr (store_type::bridges_count == 0) {
        return true;
    } else {
        using bridge_type = udho::view::data::bridges::lua;
        const auto& views = store.template tmpl<bridge_type>();
        const auto& index = views.by_composite();

        return index.find(boost::make_tuple("udho", "listing_table")) != index.end()
            && index.find(boost::make_tuple("udho", "listing_page")) != index.end()
            && index.find(boost::make_tuple("udho", "routes_page")) != index.end()
            && index.find(boost::make_tuple("udho", "header")) != index.end()
            && index.find(boost::make_tuple("udho", "status")) != index.end();
    }
}

/**
 * @brief Checks whether all views required by the system pages are registered.
 * @tparam Bridges Bridge types exposed by the read-only store.
 * @param store Read-only resource store to inspect.
 * @return True for stores without bridges, or when every system Lua view is present.
 * @ingroup DoxyG_pages
 */
template <typename... Bridges>
bool ready(const udho::view::resources::const_store<Bridges...>& store){
    using store_type = udho::view::resources::const_store<Bridges...>;

    if constexpr (store_type::bridges_count == 0) {
        return true;
    } else {
        using bridge_type = udho::view::data::bridges::lua;
        const auto views = store.template tmpl<bridge_type>();

        bool listing_table = false;
        bool listing_page  = false;
        bool routes_page   = false;
        bool header        = false;
        bool status        = false;

        for(auto it = views.begin("udho"); it != views.end("udho"); ++it) {
            listing_table = listing_table || it->name() == "listing_table";
            listing_page  = listing_page  || it->name() == "listing_page";
            routes_page   = routes_page   || it->name() == "routes_page";
            header        = header        || it->name() == "header";
            status        = status        || it->name() == "status";
        }

        return listing_table && listing_page && routes_page && header && status;
    }
}

/**
 * @brief Registers system-page Lua views in a resource store.
 * @tparam Bridges Bridge types supported by the store.
 * @param store Resource store to populate.
 * @ingroup DoxyG_pages
 */
template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store){
    using store_type = udho::view::resources::store<Bridges...>;
    static constexpr const std::size_t bridges_count = store_type::bridges_count;

    if constexpr (bridges_count > 0) {
        store["udho"] << udho::view::resources::lua{"listing_table",   std::begin(template_listing_table),  std::end(template_listing_table)}
                      << udho::view::resources::lua{"listing_page",    std::begin(template_listing_page),   std::end(template_listing_page)}
                      << udho::view::resources::lua{"routes_page",     std::begin(template_routes_page),    std::end(template_routes_page)}
                      << udho::view::resources::lua{"header",          std::begin(template_listing_header), std::end(template_listing_header)}
                      << udho::view::resources::lua{"status",          std::begin(template_listing_status), std::end(template_listing_status)};
    }
}

}
}
}
}


#endif // UDHO_PAGES_VIEWS_H
