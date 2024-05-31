#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>

struct address{
    std::string locality;

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

    inline double debt() const { return _debt; }
    inline void set_debt(const std::uint32_t& v) { _debt = v > 100 ? 100 : v; }

    std::string print(){
        std::stringstream stream;
        stream << udho::url::format("Name: {}, Age: {}, Debt: {} Addresses: ", name, age, _debt)  << std::endl;
        for(const address& addr: addresses){
            stream << addr.locality << std::endl;
        }
        return stream.str();
    }

    friend auto prototype(udho::view::data::type<person>){
        using namespace udho::view::data;

        return assoc(
            mvar("name",        &person::name),
            cvar("age",         &person::age),
            fvar("debt",        &person::debt, &person::set_debt),
            mvar("addresses",   &person::addresses),
            func("print",       &person::print)
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
    // lua.bind(udho::view::data::type<address>{});
    // lua.bind(udho::view::data::type<person>{});
    bool res = lua.compile(udho::view::resources::resource::view("user:profile", buffer, buffer+sizeof(buffer)), "");
    REQUIRE(res == true);

    person p;
    p.name = "Mr. X";
    p.age  = 25;
    p.set_debt(50);
    p.addresses.push_back(address{"Good locality"});
    p.addresses.push_back(address{"Bad locality"});
    p.addresses.push_back(address{"Far away locality"});

    std::string output;
    lua.exec("user:profile", "", p, output);

    std::cout << output << std::endl;
}
