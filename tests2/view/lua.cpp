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

TEST_CASE("Lua Interop", "[view][lua][interop]") {
    CHECK(1 == 1);

    student p;
    p.first_name = "X";
    p.last_name = "Bose";
    p.age  = 25;
    p.permanent_address = address{"Good locality"};
    p.permanent_address.zip = 7086;
    p.set_debt(50);

    // Adding courses and specializations
    education phd_education{"PhD", "JU"};
    phd_education.add_specialization(specialization{"Data Science"});
    phd_education.add_specialization(specialization{"Machine Learning"});

    phd_education.marks._marks.insert({"Data Structures", 85.0});
    phd_education.marks._marks.insert({"Algorithms", 90.0});

    education mca_education{"MCA", "SMU"};
    mca_education.add_specialization(specialization{"Software Engineering"});
    mca_education.add_specialization(specialization{"Cybersecurity"});
    mca_education.marks._marks.insert({"Object Oriented Programming", 88.0});

    education bca_education{"BCA", "SMU"};
    bca_education.add_specialization(specialization{"Information Technology"});
    bca_education.marks._marks.insert({"Database Systems", 92.0});

    p.courses.push_back(phd_education);
    p.courses.push_back(mca_education);
    p.courses.push_back(bca_education);

    static char buffer_vars[] = R"TEMPLATE(
<?! vars('d', 'ctx') whitespace(off) ?>
Name: <?= d.first_name ?> <?= d.last_name ?>
Age: <?= d.age ?>
Debt: <?= d.debt ?>
Address: <?= d.address.locality ?> (<?= d.address.zip ?>)
)TEMPLATE";

static char buffer_functions[] = R"TEMPLATE(
<?! vars('d', 'ctx') whitespace(off) ?>

<?= d:print() ?>
Result of add: <?= d:add(10, 20.5, 15.5, 5) ?>
    )TEMPLATE";

// student::cources is std::vector<education>
static char buffer_stl[] = R"TEMPLATE(
<?! vars('d', 'ctx') whitespace(on) ?>
<? for i, e in ipairs(d.courses) do ?>Course <?= i ?>: <?= e.course ?> at <?= e.university ?>
<? end ?>
)TEMPLATE";

// d.courses[1] gives const reference of an instance of education object.
// The member variable education::_specializations is private accessible through at, begin, end methods
// We first test using the ipairs method that uses the education::at in Lua 5.3, 5.4 and probably use the begin, end based __ipairs method in Lua 5.2
// Then we explicitely test the ipairs method, assuming if it works then __ipairs will also work and hence it will also work with Lua 5.2
static char buffer_iter[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>

<? for i, specialization in ipairs(d.courses[1]) do ?>Specialization <?= i ?>: <?= specialization.name ?>
<? end ?>
<? for i, specialization in d.courses[1]:ipairs() do ?>Specialization <?= i ?>: <?= specialization.name ?>
<? end ?>
)TEMPLATE";

static char buffer_assoc[] = R"TEMPLATE(
<?! vars('d', 'ctx') whitespace(off) ?>
<? for subject, marks in pairs(d.courses[1].marks) do ?><?= subject ?>: <?= marks ?>
<? end ?>
)TEMPLATE";

static char buffer_index[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
Course at index 2: <?= d.courses[2].course ?> at <?= d.courses[2].university ?>
Mark for Object Oriented Programming: <?= d.courses[2].marks["Object Oriented Programming"] ?>
)TEMPLATE";

static char buffer_all[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
Name: <?= d.first_name ?> <?= d.last_name ?>
Age: <?= d.age ?>
Debt: <?= d.debt ?>
Address: <?= d.address.locality ?> (<?= d.address.zip ?>)

<?= d:print() ?>

<? for i, e in ipairs(d.courses) do ?>Course <?= i ?>: <?= e.course ?> at <?= e.university ?>
<? end ?>

<? for i, specialization in ipairs(d.courses[1]) do ?>Specialization <?= i ?>: <?= specialization.name ?>
<? end ?>
Course at index 2: <?= d.courses[2].course ?> at <?= d.courses[2].university ?>
Mark for Object Oriented Programming: <?= d.courses[2].marks["Object Oriented Programming"] ?>
)TEMPLATE";

static char buffer_left_out[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
Number of courses: <?= #d.courses ?>
)TEMPLATE";

    const std::vector<std::pair<std::string, const char*>> views = {
        {"vars",        buffer_vars         },
        {"functions",   buffer_functions    },
        {"stl",         buffer_stl          },
        {"iter",        buffer_iter         },
        {"assoc",       buffer_assoc        },
        {"index",       buffer_index        },
        {"all",         buffer_all          },
        {"left_out",    buffer_left_out     }
    };

    const std::vector<std::string> expected_outputs = {
        "\nName: X Bose\nAge: 25.0\nDebt: 50.0\nAddress: Good locality (7086)\n",
        "\nName: X Bose, Age: 25, Debt: 50 Address: Good locality\nPhD at JU\nMCA at SMU\nBCA at SMU\n\nResult of add: 51.0\n",
        "\nCourse 1: PhD at JU\nCourse 2: MCA at SMU\nCourse 3: BCA at SMU\n\n",
        "\nSpecialization 1: Data Science\nSpecialization 2: Machine Learning\n\nSpecialization 1: Data Science\nSpecialization 2: Machine Learning\n\n",
        "\nAlgorithms: 90.0\nData Structures: 85.0\n\n",
        "\nCourse at index 2: MCA at SMU\nMark for Object Oriented Programming: 88.0\n",
        "\nName: X Bose\nAge: 25.0\nDebt: 50.0\nAddress: Good locality (7086)\n\nName: X Bose, Age: 25, Debt: 50 Address: Good locality\nPhD at JU\nMCA at SMU\nBCA at SMU\n\nCourse 1: PhD at JU\nCourse 2: MCA at SMU\nCourse 3: BCA at SMU\n\nSpecialization 1: Data Science\nSpecialization 2: Machine Learning\n\nCourse at index 2: MCA at SMU\nMark for Object Oriented Programming: 88.0\n",
        "\nNumber of courses: 3\n"
    };


    SECTION("Default bridge") {
        udho::view::data::bridges::lua lua;
        lua.init();

        std::size_t anything_as_aux;

        for (std::size_t i = 0; i < views.size(); ++i) {
            bool res = lua.compile(udho::view::resources::tmpl::resource(views[i].first, views[i].second, views[i].second + std::strlen(views[i].second)), "prefix");
            INFO("Compiling view " << views[i].first);
            REQUIRE(res == true);
        }

        for (std::size_t i = 0; i < views.size(); ++i) {
            INFO("Testing expected output for view " << views[i].first);
            std::string output;
            lua.exec(views[i].first, "prefix", p, anything_as_aux, output);
            CHECK(output == expected_outputs[i]);
        }
    }

    SECTION("Threadsafe bridge") {
        udho::view::data::bridges::lua lua{1, udho::view::data::bridges::policy::thread_safe};
        lua.init();

        for (std::size_t i = 0; i < views.size(); ++i) {
            bool res = lua.compile(udho::view::resources::tmpl::resource(views[i].first, views[i].second, views[i].second + std::strlen(views[i].second)), "prefix");
            INFO("Compiling view " << views[i].first);
            REQUIRE(res == true);
        }

        std::vector<std::thread> threads;
        std::vector<std::string> outputs{views.size()};
        std::mutex               mutex;
        std::vector<student>     data{views.size()};

        std::fill(data.begin(), data.end(), p);

        std::size_t anything_as_aux;

        for (std::size_t i = 0; i < views.size(); ++i) {
            threads.emplace_back([&lua, &views, i,  &data, &outputs, &mutex, &anything_as_aux] {
                std::string output;
                lua.exec(views[i].first, "prefix", data[i], anything_as_aux, output);
                std::lock_guard<std::mutex> lock(mutex);
                // std::cout << output << std::endl;
                outputs[i] = output;
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        for (std::size_t i = 0; i < views.size(); ++i) {
            INFO("Testing expected output for view " << views[i].first);
            CHECK(outputs[i] == expected_outputs[i]);
        }
    }

    SECTION("State Pool Bridge") {
        udho::view::data::bridges::lua lua{4, udho::view::data::bridges::policy::state_pool};
        lua.init();

        for (std::size_t i = 0; i < views.size(); ++i) {
            bool res = lua.compile(udho::view::resources::tmpl::resource(views[i].first, views[i].second, views[i].second + std::strlen(views[i].second)), "prefix");
            INFO("Compiling view " << views[i].first);
            REQUIRE(res == true);
        }

        std::vector<std::thread> threads;
        std::vector<std::string> outputs{views.size()};
        std::mutex               mutex;
        std::vector<student>     data{views.size()};

        std::fill(data.begin(), data.end(), p);

        std::size_t anything_as_aux;

        for (std::size_t i = 0; i < views.size(); ++i) {
            threads.emplace_back([&lua, &views, i,  &data, &outputs, &mutex, &anything_as_aux] {
                std::string output;
                lua.exec(views[i].first, "prefix", data[i], anything_as_aux, output);
                std::lock_guard<std::mutex> lock(mutex);
                // std::cout << output << std::endl;
                outputs[i] = output;
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        for (std::size_t i = 0; i < views.size(); ++i) {
            INFO("Testing expected output for view " << views[i].first);
            CHECK(outputs[i] == expected_outputs[i]);
        }
    }

    SECTION("State Pool Bridge Reading the same data") {
        udho::view::data::bridges::lua lua{4, udho::view::data::bridges::policy::state_pool};
        lua.init();

        for (std::size_t i = 0; i < views.size(); ++i) {
            bool res = lua.compile(udho::view::resources::tmpl::resource(views[i].first, views[i].second, views[i].second + std::strlen(views[i].second)), "prefix");
            INFO("Compiling view " << views[i].first);
            REQUIRE(res == true);
        }

        std::vector<std::thread> threads;
        std::vector<std::string> outputs{views.size()};
        std::mutex               mutex;

        std::size_t anything_as_aux;

        for (std::size_t i = 0; i < views.size(); ++i) {
            threads.emplace_back([&lua, &views, i,  &p, &outputs, &mutex, &anything_as_aux] {
                std::string output;
                lua.exec(views[i].first, "prefix", p, anything_as_aux, output);
                std::lock_guard<std::mutex> lock(mutex);
                // std::cout << output << std::endl;
                outputs[i] = output;
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        for (std::size_t i = 0; i < views.size(); ++i) {
            INFO("Testing expected output for view " << views[i].first);
            CHECK(outputs[i] == expected_outputs[i]);
        }
    }
}
