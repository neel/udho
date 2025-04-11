#ifndef UDHO_PAGES_VIEWS_H
#define UDHO_PAGES_VIEWS_H

#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <udho/view/resources/lua.h>

namespace udho{
namespace pages{
namespace system{
namespace views{

constexpr static char template_listing_files[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.css("udho", "system.css") include.css("udho", "listing.css") ?>

<div class="listing-container" id="<?= d.label ?>">
    <div class="udho-directory-container">
        <table class="udho-directory-listing">
            <thead>
                <tr>
                    <th scope="col">Name</th>
                    <th scope="col">Type</th>
                    <th scope="col">Permissions</th>
                    <th scope="col">Size</th>
                    <th scope="col">Last Modified</th>
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
                        <a href="<?= e.url ?>">
                            <span class="<?= entry_class ?>"></span>
                            <?= e.name ?>
                        </a>
                    </td>
                    <td class="<?= ext_class ?>"><?= e.extension and string.upper(e.extension) or '-' ?></td>
                    <td><code><?= e.permissions ?></code></td>
                    <td><?= e.file_size ?></td>
                    <td><?= e.modified_at ?></td>
                </tr>
            <? end ?>
            </tbody>
        </table>
    </div>
</div>

)TEMPLATE";

constexpr static char template_listing_assets[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.css("udho", "system.css") include.css("udho", "assets.css") include.css("udho", "listing.css") ?>

<div class="listing-container" id="<?= d.label ?>">
    <div class="udho-assets-container">
        <table class="udho-assets-listing">
            <thead>
                <tr>
                    <th scope="col">Name</th>
                    <th scope="col">Prefix</th>
                    <th scope="col">Type</th>
                    <th scope="col">MIME</th>
                    <th scope="col">URL</th>
                </tr>
            </thead>
            <tbody>
            <? for i, a in d:ipairs() do ?>
                <tr>
                    <?
                        local entry_class = a.is_dir and 'udho-icon-directory' or 'udho-icon-file'
                        local ext_class   = a.extension and 'udho-ext-' .. a.extension:lower() or ''
                        local type_class  = a.type and 'udho-icon-asset-type-' .. a.type or ''
                    ?>
                    <td>
                        <a href="<?= a.url ?>">
                            <span class="<?= entry_class ?>"></span>
                            <?= a.name ?>
                        </a>
                    </td>
                    <td class="udho-assets-prefix"><?= a.prefix ?></td>
                    <td class="<?= type_class ?>"><?= a.type ?></td>
                    <td><code><?= a.mime ?></code></td>
                    <td>
                        <a href="<?= a.url ?>" class="udho-asset-url">
                            <?= a.url ?>
                        </a>
                    </td>
                </tr>
            <? end ?>
            </tbody>
        </table>
    </div>
</div>

)TEMPLATE";

constexpr static char template_listing_header[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
<div class="logo">
    <img src="<?= ctx.resources.img:get('udho', 'beral.gif').url ?>" />
</div>
<div class="location">
    Index of <?= d.base ?>
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
    <li> <?= d.memory ?> </li>
    <li> <?= d.time ?> </li>
</ul>
)TEMPLATE";

template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store){
    store["udho"] << udho::view::resources::lua{"listing", std::begin(template_listing_files),  std::end(template_listing_files)}
                  << udho::view::resources::lua{"assets",  std::begin(template_listing_assets), std::end(template_listing_assets)}
                  << udho::view::resources::lua{"header",  std::begin(template_listing_header), std::end(template_listing_header)}
                  << udho::view::resources::lua{"status",  std::begin(template_listing_status), std::end(template_listing_status)};
}

}
}
}
}


#endif // UDHO_PAGES_VIEWS_H
