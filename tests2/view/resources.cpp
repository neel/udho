#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/data/data.h>
#include <udho/view/meta/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>

struct education{
    std::string course;
    std::string university;

    education() = default;
    education(const std::string c, const std::string& u): course(c), university(u) {}

    friend auto prototype(udho::view::data::type<education>){
        using namespace udho::view::data;

        return assoc("education"),
            mvar("course",      &education::course),
            mvar("university",  &education::university);
    }
};

struct address{
    std::string locality;
    std::size_t zip;

    address() = default;
    address(const std::string loc): locality(loc) {}

    friend auto prototype(udho::view::data::type<address>){
        using namespace udho::view::data;

        return assoc("address"),
            mvar("locality",  &address::locality),
            mvar("zip",       &address::zip);
    }
};

struct person{
    std::string first_name;
    std::string last_name;
    double      age;
    address     permanent_address;

    friend auto prototype(udho::view::data::type<person>){
        using namespace udho::view::data;

        return assoc("person"),
            mvar("first_name",   &person::first_name),
            mvar("last_name",    &person::last_name),
            cvar("age",          &person::age),
            mvar("address",      &person::permanent_address);
    }
};

struct student: person{
    std::vector<education> courses;

    student() = default;
    student(const student&) = delete;

    inline double debt() const { return _debt; }
    inline void set_debt(const std::uint32_t& v) {
        _debt = v > 100 ? 100 : v;
    }

    std::string print(){
        std::stringstream stream;
        stream << udho::url::format("Name: {} {}, Age: {}, Debt: {} Address: {}", first_name, last_name, age, _debt, permanent_address.locality)  << std::endl;
        for(const education& e: courses){
            stream << e.course << " at " << e.university << std::endl;
        }
        return stream.str();
    }

    double add(std::uint32_t a, double b, float c, int d){
        return a+b+c+d;
    }

    friend auto prototype(udho::view::data::type<student>){
        using namespace udho::view::data;

        return assoc("student"),
            prototype(type<person>()),
            fvar("debt",        &student::debt, &student::set_debt),
            mvar("courses",     &student::courses),
            func("print",       &student::print),
            func("add",         &student::add);
    }

    private:
        std::uint32_t _debt;
};

TEST_CASE("Tokenizer correctly builds the trie", "[trie]") {

    // View Parameters
    // name # name of the view
    // bridge = lua, js etc..
    // vars: data, context
    // cache: expiry


    static char buffer[] = R"TEMPLATE(
<?! register('cards/student', 'js') vars('d', 'ctx') cache(1d) ?>

Hi <?= d.name ?> you are <?= d.age ?> years old.
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
    bool res = lua.compile(udho::view::resources::resource::view("user:profile", buffer, buffer+sizeof(buffer)), "");
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
    //
    auto meta = prototype(udho::view::data::type<student>{});

    {
        using namespace udho::view;

        data::meta::exec(p, "debt(60)");
        REQUIRE(p.debt() == 60.0f);

        REQUIRE(data::meta::get<double>(p, "debt") == 60.0f);
        REQUIRE(p.debt() == 60.0f);

        REQUIRE(data::meta::set(p, "debt", 70));
        REQUIRE(p.debt() == 70.0f);

        data::meta::exec(p, "address.locality('Changed Locality')");
        REQUIRE(p.permanent_address.locality == "Changed Locality");
        REQUIRE(data::meta::get<std::string>(p, "address.locality") == "Changed Locality");

        data::meta::exec(p, "address('Changed Locality Twice', 123456)");

        REQUIRE(p.permanent_address.locality == "Changed Locality Twice");
        REQUIRE(p.permanent_address.zip == 123456);

        REQUIRE(data::meta::get<double>(p, "add (1, 2, 3, 4)") == 10);

        data::meta::exec(p, "courses[0].course('Daktar')");
        data::meta::exec(p, "courses[0].university('Jadavpur University')");
        REQUIRE(p.courses[0].course == "Daktar");
        REQUIRE(p.courses[0].university == "Jadavpur University");
        REQUIRE(data::meta::get<std::string>(p, "courses[0].course") == "Daktar");
        REQUIRE(data::meta::get<std::string>(p, "courses[0].university") == "Jadavpur University");

        data::meta::exec(p, "courses[1]('Master', 'Sikkim Manipal University')");

        REQUIRE(p.courses[1].course == "Master");
        REQUIRE(p.courses[1].university == "Sikkim Manipal University");
        REQUIRE(data::meta::get<std::string>(p, "courses[1].course") == "Master");
        REQUIRE(data::meta::get<std::string>(p, "courses[1].university") == "Sikkim Manipal University");
    }

    nlohmann::json p_json = udho::view::data::to_json(p);
    std::cout << p_json << std::endl;
    student q;
    q.age = p.age;
    udho::view::data::from_json(q, p_json);
    nlohmann::json q_json = udho::view::data::to_json(q);
    std::cout << q_json << std::endl;

    std::string output;
    lua.exec("user:profile", "", p, output);

    std::cout << output << std::endl;

    std::string input = R"(x.y.z_a('v1', 'v_2',:keyword); hello[24]("world", "pluto"); hello.hi[23]('pla_net'); feature.value[1].bit(on); cache.expire(-42.24)[0];)";
    // udho::view::sections::meta_parser::parse(input);
    udho::view::data::meta::detail::ast ast{input};
    udho::view::data::meta::detail::ast::print(std::cout, ast.root());
}
