#ifndef UDHO_VIEW_SECTIONS_H
#define UDHO_VIEW_SECTIONS_H

#include <iosfwd>
#include <string>
#include <cstdint>
#include <boost/algorithm/string/trim_all.hpp>

namespace udho{
namespace view{

namespace tmpl{

/**
 * @struct section
 * @brief Represents a segment or section of a template, categorized by type and containing specific content.
 *
 * This struct is utilized within the template engine to encapsulate different kinds of content parsed from templates, such as metadata, executable code, text, and so forth.
 */
struct section {
    /**
     * @enum types
     * @brief Enumerates the different types of sections that can be defined within a template.
     */
    enum types {
        none     = 0,       ///< Represents an undefined or empty section type.
        root     = 100,     ///< The root or base section of the template.
        meta     = 101,     ///< Contains metadata about the template.
        echo     = 102,     ///< For output expressions that are directly displayed.
        eval     = 103,     ///< Contains expressions or code that should be evaluated.
        embed    = 104,     ///< For embedded resources or external content.

        comment  = 201,     ///< Contains comments within the template.
        verbatim = 202,     ///< Content that should be output exactly as it is.

        text     = 300      ///< Plain text content.
    };

    /**
     * @brief Constructs a section with a specified type.
     * @param t The type of the section as defined by the types enum.
     */
    inline explicit section(types t): _type(t) {}

    /**
     * @brief Constructs a section with specified type and content.
     * @param t The type of the section.
     * @param content The content of the section.
     */
    inline section(types t, const std::string& content): _type(t), _content(content) {
        if(_type != text && _type != verbatim){
            boost::algorithm::trim_all(_content);
        }
    }

    /**
     * @brief Constructs a section with specified type and content from iterators.
     * @param t The type of the section.
     * @param begin Iterator pointing to the beginning of the content.
     * @param end Iterator pointing to the end of the content.
     */
    template <typename InputIt>
    inline section(types t, InputIt begin, InputIt end): _type(t), _content(begin, end) {
        if(_type != text && _type != verbatim){
            boost::algorithm::trim_all(_content);
        }
    }

    /**
     * @brief Returns the type of the section.
     * @return The type of the section as defined in the types enum.
     */
    inline types type() const { return _type; }
    /**
     * @brief Sets the content of the section.
     * @param c The content to set.
     */
    inline void content(const std::string& c) { _content = c; }
    /**
     * @brief Returns the content of the section.
     * @return The content of the section as a const reference to a std::string.
     */
    inline const std::string& content() const { return _content; }
    /**
     * @brief Returns an iterator to the beginning of the content.
     * @return A const iterator to the beginning of the content string.
     */
    inline std::string::const_iterator begin() const { return _content.begin(); }
    /**
     * @brief Returns an iterator to the end of the content.
     * @return A const iterator to the end of the content string.
     */
    inline std::string::const_iterator end() const { return _content.end(); }
    /**
     * @brief Returns the size of the content.
     * @return The size of the content as a std::size_t.
     */
    inline const std::size_t size() const { return _content.size(); }
    /**
     * @brief Converts a section type to its corresponding name string.
     * @param type The section type.
     * @return A string representing the name of the section type.
     */
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
    /**
     * @brief Output stream operator overload to facilitate easy printing of section details.
     * @param stream The output stream.
     * @param s The section to output.
     * @return A reference to the output stream.
     */
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
