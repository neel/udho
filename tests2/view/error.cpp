#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

#include <udho/view/bridges/exceptions.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/tmpl/substore.h>

namespace {

using lua_bridge = udho::view::data::bridges::lua;
using view_store = udho::view::resources::tmpl::substore<lua_bridge>;

void compile(view_store& views, const std::string& prefix, const std::string& name, const char* source){
    views.add(
        prefix,
        udho::view::resources::tmpl::resource{name, source, source + std::strlen(source)}
    );
}

}

TEST_CASE("Rendering errors flatten view frames and preserve their root cause", "[view][error][trace]"){
    namespace bridges = udho::view::data::bridges;

    std::exception_ptr cause = std::make_exception_ptr(std::logic_error{"root failure"});
    bridges::rendering_error child{
        bridges::rendering_frame{":errors/child", "child output", 31, {7, 9}, {}, "child traceback"},
        "root failure",
        cause
    };
    bridges::rendering_error parent{
        bridges::rendering_frame{":errors/parent", "parent output", 21, {4, 4}, {}, "parent traceback"},
        child
    };
    bridges::rendering_error grandparent{
        bridges::rendering_frame{":errors/grandparent", "grandparent output", 11, {2, 2}, {}, "grandparent traceback"},
        parent
    };

    CHECK(grandparent.frame().view() == ":errors/grandparent");
    REQUIRE(grandparent.trace().size() == 2);
    CHECK(grandparent.trace()[0].view() == ":errors/parent");
    CHECK(grandparent.trace()[1].view() == ":errors/child");
    CHECK(grandparent.diagnostic() == "root failure");
    REQUIRE(grandparent.root_cause());

    try {
        std::rethrow_exception(grandparent.root_cause());
        FAIL("Expected root cause");
    } catch(const std::logic_error& error) {
        CHECK(std::string{error.what()} == "root failure");
    }
}

TEST_CASE("View execution preserves partial output and source location", "[view][error]"){
    lua_bridge lua;
    view_store views{lua};
    const char* source = R"VIEW(<?! vars('d', 'ctx') ?>
before error
<? error('boom') ?>
after error
)VIEW";
    compile(views, "errors", "partial", source);

    std::size_t aux = 0;
    std::string output = "stale";
    auto result = lua.exec("partial", "errors", nullptr, aux, output);

    REQUIRE(result.failed());
    CHECK(output.empty());

    try {
        result.rethrow_if_failed();
        FAIL("Expected rendering_error");
    } catch(const udho::view::data::bridges::rendering_error& error) {
        const auto& frame = error.frame();
        CHECK(frame.view() == ":errors/partial");
        CHECK(frame.partial_output().find("before error") != std::string::npos);
        CHECK(frame.partial_output().find("after error") == std::string::npos);
        CHECK(frame.generated_line() > 0);
        CHECK(frame.view_lines().begin == 3);
        CHECK(frame.view_lines().end == 3);
        REQUIRE(frame.source_lines().size() == 1);
        CHECK(frame.source_lines()[0].number == 3);
        CHECK(frame.source_lines()[0].content == "<? error('boom') ?>");
        CHECK(error.diagnostic().find("boom") != std::string::npos);
        CHECK_FALSE(frame.traceback().empty());
        CHECK(frame.traceback().find(error.diagnostic()) == std::string::npos);
    }

    udho::view::resources::tmpl::proxy<lua_bridge> view{"partial", "errors", lua};
    CHECK_THROWS_AS(view.eval(nullptr, aux), udho::view::data::bridges::rendering_error);
}

TEST_CASE("Echo errors map to their complete view blocks", "[view][error][source-map]"){
    lua_bridge lua;
    view_store views{lua};
    const char* echo_source = "<?! vars('d', 'ctx') ?>\r\n"
                              "before echo\r\n"
                              "<?= dx.heading ?>\r\n";
    compile(views, "errors", "echo", echo_source);

    std::size_t aux = 0;
    std::string output;
    auto result = lua.exec("echo", "errors", nullptr, aux, output);
    REQUIRE(result.failed());

    try {
        result.rethrow_if_failed();
        FAIL("Expected rendering_error");
    } catch(const udho::view::data::bridges::rendering_error& error) {
        const auto& frame = error.frame();
        CHECK(frame.generated_line() > 0);
        CHECK(frame.view_lines().begin == 3);
        CHECK(frame.view_lines().end == 3);
        REQUIRE(frame.source_lines().size() == 1);
        CHECK(frame.source_lines()[0].number == 3);
        CHECK(frame.source_lines()[0].content == "<?= dx.heading ?>");
        CHECK(error.diagnostic().find("dx") != std::string::npos);
    }
}

TEST_CASE("View compilation failures throw std::runtime_error", "[view][error][compile]"){
    lua_bridge lua;
    view_store views{lua};
    const char* source = R"VIEW(<?! vars('d', 'ctx') ?>
<? local value = ) ?>
)VIEW";

    try {
        views.add(
            "errors",
            udho::view::resources::tmpl::resource{
                "compile",
                source,
                source + std::strlen(source)
            }
        );
        FAIL("Expected std::runtime_error");
    } catch(const udho::view::data::bridges::rendering_error&) {
        FAIL("Compilation failure must not be a rendering_error");
    } catch(const std::runtime_error& error) {
        CHECK(std::string{error.what()}.find(":errors/compile") != std::string::npos);
    }
}

TEST_CASE("View diagnostics skip called Lua chunks when locating source", "[view][error][source-map]"){
    lua_bridge lua;
    view_store views{lua};
    const char* source = R"VIEW(<?! vars('d', 'ctx') ?>
before library error
<? local compile = loadstring or load
local explode = compile("return function() error('library boom') end", "@library.lua")()
explode() ?>
)VIEW";
    compile(views, "errors", "library", source);

    std::size_t aux = 0;
    std::string output;
    auto result = lua.exec("library", "errors", nullptr, aux, output);
    REQUIRE(result.failed());

    try {
        result.rethrow_if_failed();
        FAIL("Expected rendering_error");
    } catch(const udho::view::data::bridges::rendering_error& error) {
        const auto& frame = error.frame();
        CHECK(frame.partial_output().find("before library error") != std::string::npos);
        CHECK(frame.view_lines().begin == 3);
        CHECK(frame.view_lines().end == 5);
        REQUIRE(frame.source_lines().size() == 3);
        CHECK(frame.source_lines()[0].number == 3);
        CHECK(frame.source_lines()[0].content == "<? local compile = loadstring or load");
        CHECK(frame.source_lines()[1].number == 4);
        CHECK(frame.source_lines()[1].content == "local explode = compile(\"return function() error('library boom') end\", \"@library.lua\")()");
        CHECK(frame.source_lines()[2].number == 5);
        CHECK(frame.source_lines()[2].content == "explode() ?>");
        CHECK(error.diagnostic().find("library boom") != std::string::npos);
        CHECK(frame.traceback().find("library.lua") != std::string::npos);
    }
}

TEST_CASE("Bound C++ errors retain their original exception", "[view][error][cpp]"){
    lua_bridge lua;
    view_store views{lua};
    const char* source = R"VIEW(<?! vars('d', 'ctx') ?>
before missing view
<?= d:render(nil, ctx) ?>
)VIEW";
    compile(views, "errors", "caller", source);

    udho::view::resources::tmpl::proxy<lua_bridge> missing{"missing", "errors", lua};
    std::size_t aux = 0;
    std::string output;
    auto result = lua.exec("caller", "errors", missing, aux, output);

    REQUIRE(result.failed());
    try {
        result.rethrow_if_failed();
        FAIL("Expected rendering_error");
    } catch(const udho::view::data::bridges::rendering_error& error) {
        CHECK(error.frame().view() == ":errors/caller");
        CHECK(error.frame().partial_output().find("before missing view") != std::string::npos);
        CHECK(error.frame().view_lines().begin == 3);
        REQUIRE(error.frame().source_lines().size() == 1);
        CHECK(error.frame().source_lines()[0].number == 3);
        CHECK(error.frame().source_lines()[0].content == "<?= d:render(nil, ctx) ?>");
        CHECK(error.trace().empty());
        CHECK(error.diagnostic().find("View not found") != std::string::npos);
        REQUIRE(error.root_cause());

        try {
            std::rethrow_exception(error.root_cause());
            FAIL("Expected original C++ exception");
        } catch(const udho::view::data::bridges::rendering_error&) {
            FAIL("Root cause must not be another rendering_error");
        } catch(const std::runtime_error& cause) {
            CHECK(std::string{cause.what()}.find("View not found") != std::string::npos);
        }
    }
}

TEST_CASE("Nested rendering errors retain parent and child partial output", "[view][error][nested]"){
    lua_bridge lua;
    view_store views{lua};
    const char* child_source = R"VIEW(<?! vars('d', 'ctx') ?>
child before error
<? error('nested boom') ?>
)VIEW";
    const char* parent_source = R"VIEW(<?! vars('d', 'ctx') ?>
parent before child
<?= d:render(nil, ctx) ?>
)VIEW";
    const char* healthy_source = R"VIEW(<?! vars('d', 'ctx') ?>healthy)VIEW";

    compile(views, "errors", "child", child_source);
    compile(views, "errors", "parent", parent_source);
    compile(views, "errors", "healthy", healthy_source);

    udho::view::resources::tmpl::proxy<lua_bridge> child{"child", "errors", lua};
    std::size_t aux = 0;
    std::string output;
    auto result = lua.exec("parent", "errors", child, aux, output);

    REQUIRE(result.failed());
    CHECK(output.empty());

    try {
        result.rethrow_if_failed();
        FAIL("Expected rendering_error");
    } catch(const udho::view::data::bridges::rendering_error& parent) {
        CHECK(parent.frame().view() == ":errors/parent");
        CHECK(parent.frame().partial_output().find("parent before child") != std::string::npos);
        CHECK(parent.frame().view_lines().begin == 3);
        REQUIRE(parent.frame().source_lines().size() == 1);
        CHECK(parent.frame().source_lines()[0].number == 3);
        CHECK(parent.frame().source_lines()[0].content == "<?= d:render(nil, ctx) ?>");
        CHECK(parent.diagnostic().find("nested boom") != std::string::npos);
        CHECK_FALSE(parent.root_cause());
        REQUIRE(parent.trace().size() == 1);

        const auto& child_frame = parent.trace().front();
        CHECK(child_frame.view() == ":errors/child");
        CHECK(child_frame.partial_output().find("child before error") != std::string::npos);
        CHECK(child_frame.view_lines().begin == 3);
        REQUIRE(child_frame.source_lines().size() == 1);
        CHECK(child_frame.source_lines()[0].number == 3);
        CHECK(child_frame.source_lines()[0].content == "<? error('nested boom') ?>");
    }

    // Exception propagation must leave the Lua state reusable.
    auto healthy = lua.exec("healthy", "errors", nullptr, aux, output);
    REQUIRE_FALSE(healthy.failed());
    CHECK(output == "healthy");
}
