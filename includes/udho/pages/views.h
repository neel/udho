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
                        <a href="<?= e.url ?>">
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
<?! vars('d', 'ctx') include.css("udho", "system.css") include.css("udho", "listing.css") ?>

<div class='tab-container'>
    <div class='tab-buttons'>
        <? for i, e in d:ipairs() do ?>
            <button class='tab-btn <?= (i == 1) and "active-tab" or "" ?>' data-target='<?= e.label ?>'><?= e.label ?></button>
        <? end ?>
        <div class="nav-buttons">
            <span class="nav-current">Index of / </span>
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
    store["udho"] << udho::view::resources::lua{"listing_table",   std::begin(template_listing_table),  std::end(template_listing_table)}
                  << udho::view::resources::lua{"listing_page",    std::begin(template_listing_page),   std::end(template_listing_page)}
                  << udho::view::resources::lua{"header",          std::begin(template_listing_header), std::end(template_listing_header)}
                  << udho::view::resources::lua{"status",          std::begin(template_listing_status), std::end(template_listing_status)};
}

}
}
}
}


#endif // UDHO_PAGES_VIEWS_H
