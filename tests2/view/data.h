#ifndef UDHO_TEST_VIEW_DATA_H
#define UDHO_TEST_VIEW_DATA_H

#include <string>
#include <udho/view/data.h>
#include <udho/view/meta.h>

struct specialization{
    std::string name;

    friend auto metatype(udho::view::data::type<specialization>){
        using namespace udho::view::data;

        return assoc("specialization"),
            cvar("name", &specialization::name);
    }
};

// TODO KNOWN ISSUE if there is beging end function in a class then sol2 expects the class to implement all methods and typedefs of the stl container
struct marksheet{
    using collection     = std::map<std::string, double>;

    collection _marks;

    collection::const_iterator begin() const  { return _marks.begin(); }
    collection::const_iterator end()   const  { return _marks.end();   }
    collection::size_type      size()  const  { return _marks.size();  }
    collection::mapped_type    at(collection::key_type key) const {
        return _marks.at(key);
    }

    friend auto metatype(udho::view::data::type<marksheet>){
        using namespace udho::view::data;

        return assoc("marksheet"),
            cvar("size", &marksheet::size),
            iter(&marksheet::begin, &marksheet::end),
            index(&marksheet::at, &marksheet::size);
    }

    // private:
    //     using const_iterator = collection::const_iterator;
    //     using size_type      = collection::size_type;
    //     using key_type       = collection::key_type;
    //     using value_type     = collection::value_type;
    //     using mapped_type    = collection::mapped_type;
};

struct education{
    using const_iterator = std::vector<specialization>::const_iterator;
    using size_type      = std::vector<specialization>::size_type;
    using value_type     = const specialization&;

    std::string course;
    std::string university;
    marksheet   marks;

    education() = default;
    education(const std::string c, const std::string& u): course(c), university(u) {}

    friend auto metatype(udho::view::data::type<education>){
        using namespace udho::view::data;

        return assoc("education"),
            mvar("course",      &education::course),
            mvar("university",  &education::university),
            cvar("marks",       &education::marks),
            iter(&education::begin, &education::end),
            index(&education::at, &education::size);
    }

    void add_specialization(const specialization& specialization){
        _specializations.emplace_back(specialization);
    }
    const_iterator begin() const  { return _specializations.begin(); }
    const_iterator end()   const  { return _specializations.end();   }
    size_type      size()  const  { return _specializations.size();  }
    value_type     at(std::size_t i) const {
        return _specializations.at(i);
    }

    private:
        std::vector<specialization> _specializations;
};

struct address{
    std::string locality;
    std::size_t zip;

    address() = default;
    address(const std::string loc): locality(loc) {}

    friend auto metatype(udho::view::data::type<address>){
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

    friend auto metatype(udho::view::data::type<person>){
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
    student(const student&) = default;

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

    friend auto metatype(udho::view::data::type<student>){
        using namespace udho::view::data;

        return assoc("student"),
            metatype(type<person>()),
            fvar("debt",        &student::debt, &student::set_debt),
            mvar("courses",     &student::courses),
            func("print",       &student::print),
            func("add",         &student::add);
    }

    private:
        std::uint32_t _debt;
};

#endif // UDHO_TEST_VIEW_DATA_H
