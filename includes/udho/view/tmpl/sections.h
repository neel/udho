#ifndef UDHO_VIEW_SECTIONS_H
#define UDHO_VIEW_SECTIONS_H

#include <iosfwd>
#include <string>
#include <cstdint>
#include <boost/algorithm/string/trim_all.hpp>

namespace udho{
namespace view{

namespace tmpl{

struct section {
    enum types {
        none     = 0,
        root     = 100,
        meta     = 101,
        echo     = 102,
        eval     = 103,
        embed    = 104,

        comment  = 201,
        verbatim = 202,

        text     = 300
    };

    inline explicit section(types t): _type(t) {}
    inline section(types t, const std::string& content): _type(t), _content(content) {
        if(_type != text && _type != verbatim){
            boost::algorithm::trim_all(_content);
        }
    }
    template <typename InputIt>
    inline section(types t, InputIt begin, InputIt end): _type(t), _content(begin, end) {
        if(_type != text && _type != verbatim){
            boost::algorithm::trim_all(_content);
        }
    }
    inline types type() const { return _type; }
    inline void content(const std::string& c) { _content = c; }
    inline const std::string& content() const { return _content; }
    inline std::string::const_iterator begin() const { return _content.begin(); }
    inline std::string::const_iterator end() const { return _content.end(); }
    inline const std::size_t size() const { return _content.size(); }
    inline static std::string name(const section::types type){
        switch(type){
            case section::meta:     return "meta";
            case section::echo:     return "echo";
            case section::eval:     return "eval";
            case section::embed:    return "embed";
            case section::comment:  return "comment";
            case section::text:     return "text";
            case section::verbatim: return "verbatim";
            case section::root:     return "root";
            case section::none:     return "none";
        }
        return "none";
    }

    friend std::ostream& operator<<(std::ostream& stream, const section& s){
        stream << "section " << section::name(s._type) << " >" << s._content << "< " << std::endl;
        return stream;
    }

    private:
        types _type;
        std::string _content;
};

}
}
}

#endif // UDHO_VIEW_SHORTCODE_PARSER_H
