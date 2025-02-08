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
        <? for i, e in ipairs(d) do ?>
            <tr>
                <?
                    local entry_class = e.is_dir and 'udho-icon-directory' or 'udho-icon-file'
                    local ext_class = e.extension and 'udho-ext-' .. e.extension:lower() or ''
                ?>
                <td>
                    <a href="<?= e.name ?>">
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

)TEMPLATE";

constexpr static char template_listing_header[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>

)TEMPLATE";

constexpr static char template_listing_status[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>

)TEMPLATE";

template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store){
    store["udho"] << udho::view::resources::lua{"listing", std::begin(template_listing_files),  std::end(template_listing_files)}
                  << udho::view::resources::lua{"header",  std::begin(template_listing_header), std::end(template_listing_header)}
                  << udho::view::resources::lua{"status",  std::begin(template_listing_status), std::end(template_listing_status)};
}

}
}
}
}


#endif // UDHO_PAGES_VIEWS_H
