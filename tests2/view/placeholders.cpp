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
#include <udho/url/detail/format.h>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/presenter.h>



template <typename Layout>
struct placeholder_value_printer{
    placeholder_value_printer(const Layout& layout, bool only_contents = true): _layout(layout), _only_contents(only_contents) {}

    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream){
        if(_only_contents){
            stream << str;
        } else {
            const auto& properties = _layout.properties(key);
            if(properties.isset()){
                stream << properties.open();
            }
            stream << str;
            if(properties.isset()){
                stream << properties.close();
            }
        }
    }
    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream, std::size_t i, std::size_t len){
        if(_only_contents){
            stream << str;
        } else {
            const auto& properties = _layout.properties(key);

            if(i == 0 && properties.isset()){
                stream << properties.open();
            }

            stream << str;

            if(i == len-1 && properties.isset()){
                stream << properties.close();
            }
        }
    }
    private:
        const Layout& _layout;
        bool          _only_contents;
};

enum class extra_places{
    p1,
    p2
};

enum class extra_qlaces{
    q1,
    q2
};

TEST_CASE("View layout placeholders", "[view][placeholder][layout]") {
    using namespace udho::view::tmpl::layout;

    placeholders::standard  standard_layout;
    placeholders::common    common_layout;
    placeholders::multi     multi_valued;

    SECTION("Standard placeholders operations") {
        CHECK_NOTHROW(standard_layout[placeholders::central] = "Central content");
        CHECK(standard_layout[placeholders::central].exists());
        CHECK(standard_layout[placeholders::central].count() == 1);
        CHECK(standard_layout[placeholders::central].value() == "Central content");
        CHECK_NOTHROW(*standard_layout[placeholders::central] = "Central content modified");
        CHECK(*standard_layout[placeholders::central] == "Central content modified");

        CHECK_NOTHROW(standard_layout[placeholders::header] = "Header content");
        CHECK(standard_layout[placeholders::header].exists());
        CHECK(standard_layout[placeholders::header].count() == 1);
        CHECK(standard_layout[placeholders::header].value() == "Header content");
        CHECK_NOTHROW(*standard_layout[placeholders::header] = "Header content modified");
        CHECK(*standard_layout[placeholders::header] == "Header content modified");

        CHECK_FALSE(standard_layout[placeholders::footer].exists());
        CHECK(standard_layout[placeholders::footer].count() == 0);

        CHECK_NOTHROW(standard_layout[placeholders::left] += "Left content 1");
        CHECK_NOTHROW(standard_layout[placeholders::left] += "Left content 2");
        CHECK(standard_layout[placeholders::left].exists());
        CHECK(standard_layout[placeholders::left].count() == 2);

        auto it = standard_layout[placeholders::left].begin();
        CHECK(*it == "Left content 1");
        CHECK_NOTHROW(*it = "Left content 1 modified");
        CHECK(*it == "Left content 1 modified");
        ++it;
        CHECK(*it == "Left content 2");
        CHECK_NOTHROW(*it = "Left content 2 modified");
        CHECK(*it == "Left content 2 modified");

        CHECK(standard_layout[placeholders::left][0] == "Left content 1 modified");
        CHECK(standard_layout[placeholders::left][1] == "Left content 2 modified");

        CHECK_FALSE(standard_layout[placeholders::right].exists());
        CHECK(standard_layout[placeholders::right].count() == 0);

        CHECK_THROWS_AS(standard_layout[placeholders::footer].value(), std::bad_optional_access);
    }

    SECTION("Multi-value placeholders operations") {
        CHECK_NOTHROW(multi_valued[placeholders::east] += "East content 1");
        CHECK_NOTHROW(multi_valued[placeholders::east] += "East content 2");
        CHECK(multi_valued[placeholders::east].exists());
        CHECK(multi_valued[placeholders::east].count() == 2);

        {
            auto it = multi_valued[placeholders::east].begin();
            CHECK(*it == "East content 1");
            CHECK_NOTHROW(*it = "East content 11");
            ++it;
            CHECK(*it == "East content 2");
            CHECK_NOTHROW(*it = "East content 22");
        }{
            auto it = multi_valued[placeholders::east].begin();
            CHECK(*it == "East content 11");
            ++it;
            CHECK(*it == "East content 22");
        }
    }


    SECTION("Placeholder existence and initialization") {
        CHECK_FALSE(common_layout[placeholders::north].exists());
        CHECK_NOTHROW(common_layout[placeholders::north] = "North content");
        CHECK(common_layout[placeholders::north].exists());
        CHECK(common_layout[placeholders::north].value() == "North content");
    }

    SECTION("Placeholders values and order using standard layout") {
        std::stringstream stream;
        auto printer = placeholder_value_printer{standard_layout};

        standard_layout[placeholders::central] = "C";
        standard_layout[placeholders::left] += "L1";
        standard_layout[placeholders::left] += "L2";
        standard_layout[placeholders::right] += "R1";
        standard_layout[placeholders::right] += "R2";
        standard_layout[placeholders::header] = "H";
        standard_layout[placeholders::footer] = "F";

        standard_layout.apply(printer, stream);

        CHECK(stream.str() == "HL1L2CR1R2F");
    }

    SECTION("Placeholders values, order and properties using standard layout") {
        std::stringstream stream;
        auto printer = placeholder_value_printer{standard_layout, false};

        SECTION("all properties no missing values") {
            standard_layout.properties(placeholders::central).classes("central_class").id("central_id");
            standard_layout.properties(placeholders::left).classes("left_class").id("left_id");
            standard_layout.properties(placeholders::right).classes("right_class").id("right_id");
            standard_layout.properties(placeholders::header).classes("header_class").id("header_id");
            standard_layout.properties(placeholders::footer).classes("footer_class").id("footer_id");

            standard_layout[placeholders::central] = "C";
            standard_layout[placeholders::left] += "L1";
            standard_layout[placeholders::left] += "L2";
            standard_layout[placeholders::right] += "R1";
            standard_layout[placeholders::right] += "R2";
            standard_layout[placeholders::header] = "H";
            standard_layout[placeholders::footer] = "F";

            standard_layout.apply(printer, stream);

            CHECK(stream.str() == "\
<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"left_class\" id=\"left_id\">L1L2</div>\
<div class=\"central_class\" id=\"central_id\">C</div>\
<div class=\"right_class\" id=\"right_id\">R1R2</div>\
<div class=\"footer_class\" id=\"footer_id\">F</div>");
        }

        SECTION("partial properties"){
            standard_layout.properties(placeholders::central).classes("central_class").id("central_id");
            standard_layout.properties(placeholders::right).classes("right_class").id("right_id");
            standard_layout.properties(placeholders::header).classes("header_class").id("header_id");
            standard_layout.properties(placeholders::footer).classes("footer_class").id("footer_id");

            standard_layout[placeholders::central] = "C";
            standard_layout[placeholders::left] += "L1";
            standard_layout[placeholders::left] += "L2";
            standard_layout[placeholders::right] += "R1";
            standard_layout[placeholders::right] += "R2";
            standard_layout[placeholders::header] = "H";
            standard_layout[placeholders::footer] = "F";

            standard_layout.apply(printer, stream);

            CHECK(stream.str() == "\
<div class=\"header_class\" id=\"header_id\">H</div>\
L1L2\
<div class=\"central_class\" id=\"central_id\">C</div>\
<div class=\"right_class\" id=\"right_id\">R1R2</div>\
<div class=\"footer_class\" id=\"footer_id\">F</div>");
        }

        SECTION("missing values"){
            standard_layout.properties(placeholders::central).classes("central_class").id("central_id");
            standard_layout.properties(placeholders::left).classes("left_class").id("left_id");
            standard_layout.properties(placeholders::right).classes("right_class").id("right_id");
            standard_layout.properties(placeholders::header).classes("header_class").id("header_id");
            standard_layout.properties(placeholders::footer).classes("footer_class").id("footer_id");

            standard_layout[placeholders::central] = "C";
            standard_layout[placeholders::right] += "R1";
            standard_layout[placeholders::right] += "R2";
            standard_layout[placeholders::header] = "H";
            standard_layout[placeholders::footer] = "F";

            standard_layout.apply(printer, stream);

            CHECK(stream.str() == "\
<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"central_class\" id=\"central_id\">C</div>\
<div class=\"right_class\" id=\"right_id\">R1R2</div>\
<div class=\"footer_class\" id=\"footer_id\">F</div>");
        }

        SECTION("partial properties and missing values"){
            standard_layout.properties(placeholders::central).classes("central_class").id("central_id");
            standard_layout.properties(placeholders::right).classes("right_class").id("right_id");
            standard_layout.properties(placeholders::header).classes("header_class").id("header_id");
            standard_layout.properties(placeholders::footer).classes("footer_class").id("footer_id");

            standard_layout[placeholders::central] = "C";
            standard_layout[placeholders::right] += "R1";
            standard_layout[placeholders::right] += "R2";
            standard_layout[placeholders::header] = "H";
            standard_layout[placeholders::footer] = "F";

            standard_layout.apply(printer, stream);

            CHECK(stream.str() == "\
<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"central_class\" id=\"central_id\">C</div>\
<div class=\"right_class\" id=\"right_id\">R1R2</div>\
<div class=\"footer_class\" id=\"footer_id\">F</div>");
        }
    }

    SECTION("Placeholders values and order using common layout") {
        std::stringstream stream;
        auto printer = placeholder_value_printer{common_layout};

        common_layout[placeholders::main] = "C";
        common_layout[placeholders::west] = "L";
        common_layout[placeholders::east] = "R";
        common_layout[placeholders::north] = "H";
        common_layout[placeholders::south] = "F";

        common_layout.apply(printer, stream);

        CHECK(stream.str() == "HLCRF");
    }

    SECTION("Placeholders values, order and properties using common layout") {
        std::stringstream stream;
        auto printer = placeholder_value_printer{common_layout, false};

        SECTION("all properties no missing values") {
            common_layout.properties(placeholders::main).classes("main_class").id("main_id");
            common_layout.properties(placeholders::west).classes("west_class").id("west_id");
            common_layout.properties(placeholders::east).classes("east_class").id("east_id");
            common_layout.properties(placeholders::north).classes("north_class").id("north_id");
            common_layout.properties(placeholders::south).classes("south_class").id("south_id");

            common_layout[placeholders::main] = "C";
            common_layout[placeholders::west] = "L";
            common_layout[placeholders::east] = "R";
            common_layout[placeholders::north] = "H";
            common_layout[placeholders::south] = "F";

            common_layout.apply(printer, stream);

            CHECK(stream.str() == "\
<div class=\"north_class\" id=\"north_id\">H</div>\
<div class=\"west_class\" id=\"west_id\">L</div>\
<div class=\"main_class\" id=\"main_id\">C</div>\
<div class=\"east_class\" id=\"east_id\">R</div>\
<div class=\"south_class\" id=\"south_id\">F</div>");
        }

        SECTION("partial properties") {
            common_layout.properties(placeholders::main).classes("central_class").id("central_id");
            common_layout.properties(placeholders::west).classes("left_class").id("left_id");
            common_layout.properties(placeholders::north).classes("header_class").id("header_id");

            common_layout[placeholders::main] = "C";
            common_layout[placeholders::west] = "L";
            common_layout[placeholders::east] = "R";
            common_layout[placeholders::north] = "H";
            common_layout[placeholders::south] = "F";

            common_layout.apply(printer, stream);

            CHECK(stream.str() == "<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"left_class\" id=\"left_id\">L</div>\
<div class=\"central_class\" id=\"central_id\">C</div>\
R\
F");
        }

        SECTION("missing values") {
            common_layout.properties(placeholders::main).classes("main_class").id("main_id");
            common_layout.properties(placeholders::west).classes("west_class").id("west_id");
            common_layout.properties(placeholders::east).classes("east_class").id("east_id");
            common_layout.properties(placeholders::north).classes("north_class").id("north_id");
            common_layout.properties(placeholders::south).classes("south_class").id("south_id");

            common_layout[placeholders::main] = "C";
            common_layout[placeholders::west] = "L";
            common_layout[placeholders::east] = "R";
            common_layout[placeholders::north] = "H";

            common_layout.apply(printer, stream);

            CHECK(stream.str() == "<div class=\"north_class\" id=\"north_id\">H</div>\
<div class=\"west_class\" id=\"west_id\">L</div>\
<div class=\"main_class\" id=\"main_id\">C</div>\
<div class=\"east_class\" id=\"east_id\">R</div>");
        }

        SECTION("partial properties and missing values") {
            common_layout.properties(placeholders::main).classes("central_class").id("central_id");
            common_layout.properties(placeholders::west).classes("left_class").id("left_id");
            common_layout.properties(placeholders::north).classes("header_class").id("header_id");

            common_layout[placeholders::main] = "C";
            common_layout[placeholders::north] = "H";

            common_layout.apply(printer, stream);

            CHECK(stream.str() == "<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"central_class\" id=\"central_id\">C</div>");
        }
    }

    SECTION("Placeholders values and order using multi_valued layout") {
        std::stringstream stream;
        auto printer = placeholder_value_printer{multi_valued};

        multi_valued[placeholders::main] += "C";
        multi_valued[placeholders::west] += "L";
        multi_valued[placeholders::east] += "R";
        multi_valued[placeholders::north] += "H";
        multi_valued[placeholders::south] += "F";

        multi_valued.apply(printer, stream);

        CHECK(stream.str() == "HLCRF");
    }

    SECTION("Placeholders values, order and properties using multi_valued layout") {
        std::stringstream stream;
        auto printer = placeholder_value_printer{multi_valued, false};

        SECTION("all properties no missing values") {
            multi_valued.properties(placeholders::main).classes("main_class").id("main_id");
            multi_valued.properties(placeholders::west).classes("west_class").id("west_id");
            multi_valued.properties(placeholders::east).classes("east_class").id("east_id");
            multi_valued.properties(placeholders::north).classes("north_class").id("north_id");
            multi_valued.properties(placeholders::south).classes("south_class").id("south_id");

            multi_valued[placeholders::main] += "C";
            multi_valued[placeholders::west] += "L";
            multi_valued[placeholders::east] += "R";
            multi_valued[placeholders::north] += "H";
            multi_valued[placeholders::south] += "F";

            multi_valued.apply(printer, stream);

            CHECK(stream.str() == "\
<div class=\"north_class\" id=\"north_id\">H</div>\
<div class=\"west_class\" id=\"west_id\">L</div>\
<div class=\"main_class\" id=\"main_id\">C</div>\
<div class=\"east_class\" id=\"east_id\">R</div>\
<div class=\"south_class\" id=\"south_id\">F</div>");
        }

        SECTION("partial properties") {
            multi_valued.properties(placeholders::main).classes("central_class").id("central_id");
            multi_valued.properties(placeholders::west).classes("left_class").id("left_id");
            multi_valued.properties(placeholders::north).classes("header_class").id("header_id");

            multi_valued[placeholders::main] += "C";
            multi_valued[placeholders::west] += "L";
            multi_valued[placeholders::east] += "R";
            multi_valued[placeholders::north] += "H";
            multi_valued[placeholders::south] += "F";

            multi_valued.apply(printer, stream);

            CHECK(stream.str() == "<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"left_class\" id=\"left_id\">L</div>\
<div class=\"central_class\" id=\"central_id\">C</div>\
R\
F");
        }

        SECTION("missing values") {
            multi_valued.properties(placeholders::main).classes("main_class").id("main_id");
            multi_valued.properties(placeholders::west).classes("west_class").id("west_id");
            multi_valued.properties(placeholders::east).classes("east_class").id("east_id");
            multi_valued.properties(placeholders::north).classes("north_class").id("north_id");
            multi_valued.properties(placeholders::south).classes("south_class").id("south_id");

            multi_valued[placeholders::main] += "C";
            multi_valued[placeholders::west] += "L";
            multi_valued[placeholders::east] += "R";
            multi_valued[placeholders::north] += "H";

            multi_valued.apply(printer, stream);

            CHECK(stream.str() == "<div class=\"north_class\" id=\"north_id\">H</div>\
<div class=\"west_class\" id=\"west_id\">L</div>\
<div class=\"main_class\" id=\"main_id\">C</div>\
<div class=\"east_class\" id=\"east_id\">R</div>");
        }

        SECTION("partial properties and missing values") {
            multi_valued.properties(placeholders::main).classes("central_class").id("central_id");
            multi_valued.properties(placeholders::west).classes("left_class").id("left_id");
            multi_valued.properties(placeholders::north).classes("header_class").id("header_id");

            multi_valued[placeholders::main] += "C";
            multi_valued[placeholders::north] += "H";

            multi_valued.apply(printer, stream);

            CHECK(stream.str() == "<div class=\"header_class\" id=\"header_id\">H</div>\
<div class=\"central_class\" id=\"central_id\">C</div>");
        }
    }

    SECTION("Mixed placeholder") {
        using sub_standard = basic_placeholder<
            spot<placeholders::segments::header>,
            multispot<placeholders::segments::central>,
            spot<placeholders::segments::footer>,
            spot<extra_places>,
            multispot<extra_qlaces>
        >;

        sub_standard sub_standard_layout;

        std::stringstream stream;
        auto printer = placeholder_value_printer{sub_standard_layout};

        sub_standard_layout[placeholders::central] += "C1";
        sub_standard_layout[placeholders::central] += "C2";
        sub_standard_layout[placeholders::header] = "H";
        sub_standard_layout[placeholders::footer] = "F";
        sub_standard_layout[extra_places::p1] = "p1";
        sub_standard_layout[extra_places::p2] = "p2";
        sub_standard_layout[extra_qlaces::q1] += "q11";
        sub_standard_layout[extra_qlaces::q1] += "q12";
        sub_standard_layout[extra_qlaces::q2] += "q21";
        sub_standard_layout[extra_qlaces::q2] += "q22";

        sub_standard_layout.apply(printer, stream);

        CHECK(stream.str() == "HC1C2Fp1p2q11q12q21q22");
    }
}
