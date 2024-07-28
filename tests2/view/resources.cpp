#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>

struct address{
    std::string locality;

    address() = default;
    address(const std::string loc): locality(loc) {}

    friend auto prototype(udho::view::data::type<address>){
        using namespace udho::view::data;

        return assoc(
            mvar("locality",  &address::locality)
        ).as("address");
    }
};

struct person{
    std::string name;
    double      age;
    std::uint32_t _debt;
    std::vector<address> addresses;

    person() = default;
    person(const person&) = delete;

    inline double debt() const { return _debt; }
    inline void set_debt(const std::uint32_t& v) {
        _debt = v > 100 ? 100 : v;
    }

    std::string print(){
        std::stringstream stream;
        stream << udho::url::format("Name: {}, Age: {}, Debt: {} Addresses: ", name, age, _debt)  << std::endl;
        for(const address& addr: addresses){
            stream << addr.locality << std::endl;
        }
        return stream.str();
    }

    double add(std::uint32_t a, double b, float c, int d){
        return a+b+c+d;
    }
    friend auto prototype(udho::view::data::type<person>){
        using namespace udho::view::data;

        return assoc(
            mvar("name",        &person::name),
            cvar("age",         &person::age),
            fvar("debt",        &person::debt, &person::set_debt),
            mvar("addresses",   &person::addresses),
            func("print",       &person::print),
            func("add",         &person::add)
        ).as("person");
    }
};

TEST_CASE("Tokenizer correctly builds the trie", "[trie]") {

    static char buffer[] = R"TEMPLATE(
<?! data d; bridge 'lua'; lang 'lua' ?>
Hi <?= d.name ?> you are <?= d.age ?> years old.
You have lived in these localities:
<? for i, address in ipairs(d.addresses) do ?>
    Locality <?= i ?>: <?= address.locality ?>

<? end ?>
Your total debt is <?= d.debt ?>

-----------
<?= d:print() ?>
    )TEMPLATE";

    udho::view::data::bridges::lua lua;
    lua.init();
    bool res = lua.compile(udho::view::resources::resource::view("user:profile", buffer, buffer+sizeof(buffer)), "");
    REQUIRE(res == true);

    person p;
    p.name = "Mr. X";
    p.age  = 25;
    p.set_debt(50);
    p.addresses.emplace_back(address{"Good locality"});
    p.addresses.emplace_back(address{"Bad locality"});
    p.addresses.emplace_back(address{"Far away locality"});

    // using variant = boost::variant<std::int64_t, double, std::string, bool>;
    //
    auto meta = prototype(udho::view::data::type<person>{});

    {
        udho::view::data::detail::id_ast ast{"debt(60)"};
        bool result;
        udho::view::data::detail::id_value_extractor<bool> extractor{result};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_extractor<bool>> finder{ast.root()->children[0], p, extractor};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "set debt: " << extractor.assigned() << " " << result << std::endl;
    }{
        udho::view::data::detail::id_ast ast{"debt"};
        double result;
        udho::view::data::detail::id_value_extractor<double> extractor{result};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_extractor<double>> finder{ast.root()->children[0], p, extractor};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "extracted debt: " << extractor.assigned() << " " << result << std::endl;
    }{
        udho::view::data::detail::id_ast ast{"debt"};
        double value = 70;
        udho::view::data::detail::id_value_manipulator<double> manipulator{value};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_manipulator<double>> finder{ast.root()->children[0], p, manipulator};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "set debt: " << manipulator.assigned() << " " << value << std::endl;
    }{
        udho::view::data::detail::id_ast ast{"debt"};
        double result;
        udho::view::data::detail::id_value_extractor<double> extractor{result};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_extractor<double>> finder{ast.root()->children[0], p, extractor};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "extracted debt: " << extractor.assigned() << " " << result << std::endl;
    }{
        udho::view::data::detail::id_ast ast{"addresses[1].locality('Changed Locality')"};
        bool result;
        udho::view::data::detail::id_value_extractor<bool> extractor{result};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_extractor<bool>> finder{ast.root()->children[0], p, extractor};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "extracted locality: " << extractor.assigned() << " " << result << std::endl;
    }{
        udho::view::data::detail::id_ast ast{"addresses[1].locality"};
        std::string result;
        udho::view::data::detail::id_value_extractor<std::string> extractor{result};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_extractor<std::string>> finder{ast.root()->children[0], p, extractor};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "extracted locality: " << extractor.assigned() << " " << result << std::endl;
    }{
        udho::view::data::detail::id_ast ast{"add (1, 2, 3, 4)"};
        double result;
        udho::view::data::detail::id_value_extractor<double> extractor{result};
        udho::view::data::detail::id_finder<person, udho::view::data::detail::id_value_extractor<double>> finder{ast.root()->children[0], p, extractor};
        std::cout << "apply_ " << std::endl << meta.members().apply_(finder) << std::endl;
        std::cout << "extracted add: " << extractor.assigned() << " " << result << std::endl;
    }

    nlohmann::json p_json = udho::view::data::to_json(p);
    std::cout << p_json << std::endl;
    person q;
    q.age = p.age;
    udho::view::data::from_json(q, p_json);
    nlohmann::json q_json = udho::view::data::to_json(q);
    std::cout << q_json << std::endl;

    std::string output;
    lua.exec("user:profile", "", p, output);

    std::cout << output << std::endl;

    std::string input = R"(x.y.z_a('v1', 'v_2',:keyword); hello ("world"); hello_hi('pla_net');feature(on);cache.expire(-42.24);)";
    udho::view::sections::meta_parser::parse(input);
}
