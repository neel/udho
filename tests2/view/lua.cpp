#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>

#include "data.h"

TEST_CASE("Lua bridge", "[lua]") {

    static char buffer[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>

Hi <?= d.first_name ?> <?= d.last_name ?> you are <?= d.age ?> years old.
<? for i, e in ipairs(d.courses) do ?>
    Course <?= i ?>: <?= e.course ?> at <?= e.university ?>

<? end ?>
Your total debt is <?= d.debt ?>

Address: <?= d.address.locality ?> (<?= d.address.zip ?>)
-----------
<?= d:print() ?>
    )TEMPLATE";

    udho::view::data::bridges::lua lua;
    lua.init();
    bool res = lua.compile(udho::view::resources::tmpl::resource("user:profile", buffer, buffer+sizeof(buffer)), "");
    REQUIRE(res == true);

    student p;
    p.first_name = "X";
    p.last_name = "Bose";
    p.age  = 25;
    p.permanent_address = address{"Good locality"};
    p.permanent_address.zip = 7086;
    p.set_debt(50);
    p.courses.emplace_back(education{"PhD", "JU"});
    p.courses.emplace_back(education{"MCA", "SMU"});
    p.courses.emplace_back(education{"BCA", "SMU"});

    // using variant = boost::variant<std::int64_t, double, std::string, bool>;

    std::string output;
    lua.exec("user:profile", "", p, p, output);

    std::cout << output << std::endl;
}
