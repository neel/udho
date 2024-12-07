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


TEST_CASE("data accessible through metatype", "[view.meta]") {

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

    {
        using namespace udho::view;

        data::meta::exec(p, "debt(60)");
        CHECK(p.debt() == 60.0f);

        CHECK(data::meta::get<double>(p, "debt") == 60.0f);
        CHECK(p.debt() == 60.0f);

        CHECK(data::meta::set(p, "debt", 70));
        CHECK(p.debt() == 70.0f);

        data::meta::exec(p, "address.locality('Changed Locality')");
        CHECK(p.permanent_address.locality == "Changed Locality");
        CHECK(data::meta::get<std::string>(p, "address.locality") == "Changed Locality");

        data::meta::exec(p, "address('Changed Locality Twice', 123456)");

        CHECK(p.permanent_address.locality == "Changed Locality Twice");
        CHECK(p.permanent_address.zip == 123456);

        CHECK(data::meta::get<double>(p, "add (1, 2, 3, 4)") == 10);

        data::meta::exec(p, "courses[0].course('Daktar')");
        data::meta::exec(p, "courses[0].university('Jadavpur University')");
        CHECK(p.courses[0].course == "Daktar");
        CHECK(p.courses[0].university == "Jadavpur University");
        CHECK(data::meta::get<std::string>(p, "courses[0].course") == "Daktar");
        CHECK(data::meta::get<std::string>(p, "courses[0].university") == "Jadavpur University");

        data::meta::exec(p, "courses[1]('Master', 'Sikkim Manipal University')");

        CHECK(p.courses[1].course == "Master");
        CHECK(p.courses[1].university == "Sikkim Manipal University");
        CHECK(data::meta::get<std::string>(p, "courses[1].course") == "Master");
        CHECK(data::meta::get<std::string>(p, "courses[1].university") == "Sikkim Manipal University");
    }

    {
        nlohmann::json p_json = udho::view::data::to_json(p);
        student q;
        q.age = p.age;
        udho::view::data::from_json(q, p_json);
        nlohmann::json q_json = udho::view::data::to_json(q);

        CHECK(p_json == q_json);
    }

}

TEST_CASE("AST Generation", "[view.meta.lang]") {
    std::string input = R"(
        x.y.z_a('v1', 'v_2',:keyword);
        hello[24]("world", "pluto");
        hello.hi[23]('pla_net')
        feature.value[1].bit(false);
        cache.expire(-42.24)[0];
    )";
    udho::view::data::meta::detail::ast ast{input};
    // udho::view::data::meta::detail::ast::print(std::cout, ast.root());

    const udho::view::data::meta::detail::ast::node_ptr_type& root = ast.root();
    CHECK(root->children.size() == 1);

    const udho::view::data::meta::detail::ast::node_ptr_type& grammer = root->children[0];
    CHECK(grammer->template is_type<udho::view::data::meta::detail::ast::grammar>());
    CHECK(grammer->children.size() == 5);

    {
        const udho::view::data::meta::detail::ast::node_ptr_type& statement = grammer->children[0];
        CHECK(statement->template is_type<udho::view::data::meta::detail::ast::statement>());
        CHECK(statement->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[0]->has_content());
        CHECK(statement->children[0]->string() == "x");

        CHECK(statement->children[1]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[1]->children.size() == 1);
        CHECK(statement->children[1]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[1]->children[0]->has_content());
        CHECK(statement->children[1]->children[0]->string() == "y");

        CHECK(statement->children[2]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[2]->children.size() == 1);
        CHECK(statement->children[2]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[2]->children[0]->has_content());
        CHECK(statement->children[2]->children[0]->string() == "z_a");

        CHECK(statement->children[3]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[3]->children.size() == 1);
        CHECK(statement->children[3]->children[0]->template is_type<udho::view::data::meta::detail::ast::call>());
        CHECK(statement->children[3]->children[0]->has_content());
        CHECK(statement->children[3]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::values>());
        CHECK(statement->children[3]->children[0]->children[0]->children.size() == 3);
        CHECK(statement->children[3]->children[0]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::quoted_string>());
        CHECK(statement->children[3]->children[0]->children[0]->children[0]->string() == "'v1'");
        CHECK(statement->children[3]->children[0]->children[0]->children[1]->template is_type<udho::view::data::meta::detail::ast::quoted_string>());
        CHECK(statement->children[3]->children[0]->children[0]->children[1]->string() == "'v_2'");
        CHECK(statement->children[3]->children[0]->children[0]->children[2]->template is_type<udho::view::data::meta::detail::ast::reference>());
        CHECK(statement->children[3]->children[0]->children[0]->children[2]->children.size() == 1);
        CHECK(statement->children[3]->children[0]->children[0]->children[2]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[3]->children[0]->children[0]->children[2]->children[0]->string() == "keyword");
    }{
        const udho::view::data::meta::detail::ast::node_ptr_type& statement = grammer->children[1];
        CHECK(statement->template is_type<udho::view::data::meta::detail::ast::statement>());
        CHECK(statement->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[0]->has_content());
        CHECK(statement->children[0]->string() == "hello");

        CHECK(statement->children[1]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[1]->children.size() == 1);
        CHECK(statement->children[1]->children[0]->template is_type<udho::view::data::meta::detail::ast::at>());
        CHECK(statement->children[1]->children[0]->has_content());
        CHECK(statement->children[1]->children[0]->string() == "24");

        CHECK(statement->children[2]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[2]->children.size() == 1);
        CHECK(statement->children[2]->children[0]->template is_type<udho::view::data::meta::detail::ast::call>());
        CHECK(statement->children[2]->children[0]->has_content());
        CHECK(statement->children[2]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::values>());
        CHECK(statement->children[2]->children[0]->children[0]->children.size() == 2);
        CHECK(statement->children[2]->children[0]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::quoted_string>());
        CHECK(statement->children[2]->children[0]->children[0]->children[0]->string() == "\"world\"");
        CHECK(statement->children[2]->children[0]->children[0]->children[1]->template is_type<udho::view::data::meta::detail::ast::quoted_string>());
        CHECK(statement->children[2]->children[0]->children[0]->children[1]->string() == "\"pluto\"");
    }{
        const udho::view::data::meta::detail::ast::node_ptr_type& statement = grammer->children[2];
        CHECK(statement->template is_type<udho::view::data::meta::detail::ast::statement>());
        CHECK(statement->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[0]->has_content());
        CHECK(statement->children[0]->string() == "hello");

        CHECK(statement->children[1]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[1]->children.size() == 1);
        CHECK(statement->children[1]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[1]->children[0]->has_content());
        CHECK(statement->children[1]->children[0]->string() == "hi");

        CHECK(statement->children[2]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[2]->children.size() == 1);
        CHECK(statement->children[2]->children[0]->template is_type<udho::view::data::meta::detail::ast::at>());
        CHECK(statement->children[2]->children[0]->has_content());
        CHECK(statement->children[2]->children[0]->string() == "23");

        CHECK(statement->children[3]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[3]->children.size() == 1);
        CHECK(statement->children[3]->children[0]->template is_type<udho::view::data::meta::detail::ast::call>());
        CHECK(statement->children[3]->children[0]->has_content());
        CHECK(statement->children[3]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::values>());
        CHECK(statement->children[3]->children[0]->children[0]->children.size() == 1);
        CHECK(statement->children[3]->children[0]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::quoted_string>());
        CHECK(statement->children[3]->children[0]->children[0]->children[0]->string() == "'pla_net'");
    }{
        const udho::view::data::meta::detail::ast::node_ptr_type& statement = grammer->children[3];
        CHECK(statement->template is_type<udho::view::data::meta::detail::ast::statement>());
        CHECK(statement->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[0]->has_content());
        CHECK(statement->children[0]->string() == "feature");

        CHECK(statement->children[1]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[1]->children.size() == 1);
        CHECK(statement->children[1]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[1]->children[0]->has_content());
        CHECK(statement->children[1]->children[0]->string() == "value");

        CHECK(statement->children[2]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[2]->children.size() == 1);
        CHECK(statement->children[2]->children[0]->template is_type<udho::view::data::meta::detail::ast::at>());
        CHECK(statement->children[2]->children[0]->has_content());
        CHECK(statement->children[2]->children[0]->string() == "1");

        CHECK(statement->children[3]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[3]->children.size() == 1);
        CHECK(statement->children[3]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[3]->children[0]->has_content());
        CHECK(statement->children[3]->children[0]->string() == "bit");

        CHECK(statement->children[4]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[4]->children.size() == 1);
        CHECK(statement->children[4]->children[0]->template is_type<udho::view::data::meta::detail::ast::call>());
        CHECK(statement->children[4]->children[0]->has_content());
        CHECK(statement->children[4]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::values>());
        CHECK(statement->children[4]->children[0]->children[0]->children.size() == 1);
        CHECK(statement->children[4]->children[0]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::boolean>());
        CHECK(statement->children[4]->children[0]->children[0]->children[0]->string() == "false");
    }{
        const udho::view::data::meta::detail::ast::node_ptr_type& statement = grammer->children[4];
        CHECK(statement->template is_type<udho::view::data::meta::detail::ast::statement>());
        CHECK(statement->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[0]->has_content());
        CHECK(statement->children[0]->string() == "cache");

        CHECK(statement->children[1]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[1]->children.size() == 1);
        CHECK(statement->children[1]->children[0]->template is_type<udho::view::data::meta::detail::ast::key>());
        CHECK(statement->children[1]->children[0]->has_content());
        CHECK(statement->children[1]->children[0]->string() == "expire");

        CHECK(statement->children[2]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[2]->children.size() == 1);
        CHECK(statement->children[2]->children[0]->template is_type<udho::view::data::meta::detail::ast::call>());
        CHECK(statement->children[2]->children[0]->has_content());
        CHECK(statement->children[2]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::values>());
        CHECK(statement->children[2]->children[0]->children[0]->children.size() == 1);
        CHECK(statement->children[2]->children[0]->children[0]->children[0]->template is_type<udho::view::data::meta::detail::ast::real>());
        CHECK(statement->children[2]->children[0]->children[0]->children[0]->string() == "-42.24");

        CHECK(statement->children[3]->template is_type<udho::view::data::meta::detail::ast::index>());
        CHECK(statement->children[3]->children.size() == 1);
        CHECK(statement->children[3]->children[0]->template is_type<udho::view::data::meta::detail::ast::at>());
        CHECK(statement->children[3]->children[0]->has_content());
        CHECK(statement->children[3]->children[0]->string() == "0");
    }

}
