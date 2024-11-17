#ifndef UDHO_TEST_VIEW_DATA_H
#define UDHO_TEST_VIEW_DATA_H

#include <string>
#include <udho/view/data.h>
#include <udho/view/meta.h>

struct education{
    std::string course;
    std::string university;

    education() = default;
    education(const std::string c, const std::string& u): course(c), university(u) {}

    friend auto metatype(udho::view::data::type<education>){
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
