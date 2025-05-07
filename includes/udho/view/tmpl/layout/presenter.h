#ifndef UDHO_VIEW_LAYOUT_PRESENTER_H
#define UDHO_VIEW_LAYOUT_PRESENTER_H

#include <string>
#include <udho/url/detail/format.h>
#include <udho/view/tmpl/layout/property_map.h>
#include <udho/view/tmpl/layout/property_tree.h>
#include <udho/view/tmpl/layout/placeholder.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

/**
 * @brief provides basic functionalities for rendering a layout document but does not perform complete rendering of the document.
 * Presenting the layout document involves composing an HTML document incorporating not only the contents but also the meta contents
 * such as the title of the page, meta tags, necessary assets (e.g. js, css) which are to be included from the client side rendering.
 * While the rendering of contents may vary significantly among implementations, the rendering of meta contents is often more consistent.
 * So, in this class the basic funationalities are provided to render the meta contents. A subclass can inherit and provide functionalities for
 * rendering the contents while utilizing the common functionalities for rendering the meta contents.
 *
 * The basic functionalities include following:
 * 1. rendering opening `<html>` tag with the attributes to the provided stream
 * 2. rendering closing `</html>` tag to the provided stream
 * 3. rendering `<title>` tag to the provided stream
 * 4. rendering all `<meta>` tags to the provided stream
 * 5. rendering js and css includes to the provided stream
 * 6. rendering the `<head>...</head>` block to the provided stream incorporating 3-5
 *
 * This class fetches information from the document's preamble and renders corresponding HTML,
 * which serves as a foundation for subclasses to compose the final HTML document, such as the \ref default_presenter.
 */
template <typename DocumentT>
struct basic_presenter{
    using document_type = DocumentT;
    using basic_presenter_ = basic_presenter<DocumentT>;

    basic_presenter(const document_type& document): _document(document) {}
    private:
        const document_type& _document;
    public:
        const document_type& document() const { return _document; }
    protected:
        template <typename StreamT>
        StreamT& html_open(StreamT& stream) const {
            if (_document.preamble().doctype()) {
                stream << "<!doctype html>";
            }
            stream << "<html";
            if (!_document.preamble().doclang().empty()) {
                stream << " lang=\"" << _document.preamble().doclang() << "\"";
            }
            if (!_document.preamble().xmlns().empty()) {
                stream << " xmlns=\"" << _document.preamble().xmlns() << "\"";
            }
            if (!_document.preamble().dir().empty()) {
                stream << " dir=\"" << _document.preamble().dir() << "\"";
            }
            if (!_document.preamble().classes().empty()) {
                stream << " class=\"" << _document.preamble().classes() << "\"";
            }
            stream << ">";
            return stream;
        }
        template <typename StreamT>
        StreamT& html_close(StreamT& stream) const {
            stream << "</html>";
            return stream;
        }
        template <typename StreamT>
        StreamT& body_open(StreamT& stream) const {
            stream << _document.body().open();
            return stream;
        }
        template <typename StreamT>
        StreamT& body_close(StreamT& stream) const {
            _document.js().write_embedded(stream);
            stream << _document.body().close();
            return stream;
        }
        template <typename StreamT>
        StreamT& title(StreamT& stream) const {
            stream << "<title>" << _document.preamble().title() << "</title>";
            return stream;
        }
        template <typename StreamT>
        StreamT& meta(StreamT& stream) const {
            _document.preamble().meta.write(stream);
            return stream;
        }
        template <typename StreamT>
        StreamT& includes(StreamT& stream) const {
            _document.js().importmap(stream);
            _document.js().write(stream);
            _document.css().write(stream);
            _document.css().write_embedded(stream);
            return stream;
        }
        template <typename StreamT>
        StreamT& head(StreamT& stream) const {
            stream << "<head>";
            title(stream);
            meta(stream);
            includes(stream);
            stream << "</head>";
            return stream;
        }

    protected:
        template <typename KeyT, typename Stream>
        void present_all(const KeyT& key, Stream& stream) const {
            std::size_t len = _document[key].count();
            if(len > 0){
                for(std::size_t i = 0; i != len; ++i) {
                    present(key, *_document[key], stream, i, len);
                }
            }
        }

        template <typename KeyT, typename Stream>
        void present(const KeyT& key, Stream& stream) const {
            if(_document[key].exists())
                present(key, *_document[key], stream);
        }

    protected:
        template <typename KeyT, typename Stream>
        void present(const KeyT& key, const std::string& str, Stream& stream) const {
            const auto& properties = document().properties(key);
            bool tag_opened = false;
            if(properties.isset()){
                stream << properties.open();
                tag_opened = true;
            }
            stream << str;
            if(tag_opened){
                stream << properties.close();
            }
        }


        template <typename KeyT, typename Stream>
        void present(const KeyT& key, const std::string& str, Stream& stream, std::size_t i, std::size_t len) const {
            const auto& properties = document().properties(key);

            if(i == 0 && properties.isset()){
                stream << properties.open();
            }

            stream << str;

            if(i == len-1 && properties.isset()){
                stream << properties.close();
            }
        }
};

/**
 * @brief provides a default implementation of a presenter presenting a layout document.
 *
 * It can be customized by subclassing from it in a CRTP fasion. When the derived subclass is provided as the last template parameter
 * it calls the `render` method of that class with the provided stream while rending the body. The other parts of the HTML such as head
 * and the inner contents including js and css asset dependencies, opening and closing of the html tag etc.. is composed using the common
 * functionalities provided by basic_presenter. If no derived class is provided as the last template parameter then it renders the body by
 * iterating over the contents of all placeholders in the same order they were added in the \ref basic_placeholder. Then it uses the properties
 * such as id, classes etc set to these placeholders while rendering. It provides operator() overload that accepts a stream on which it renders
 * the composed html.
 */
template <class DocumentT, class Derived = void>
struct default_presenter;

template <class DocumentT, class Derived>
struct default_presenter: basic_presenter<DocumentT>{
    using basic_presenter_ = basic_presenter<DocumentT>;

    default_presenter(const DocumentT& doc): basic_presenter_(doc) {}

    friend DocumentT;

    template <typename Stream>
    Stream& operator()(Stream& stream) const {
        basic_presenter_::html_open(stream);
            basic_presenter_::head(stream);
            basic_presenter_::body_open(stream);
                static_cast<const Derived*>(this)->render(stream);
            basic_presenter_::body_close(stream);
        basic_presenter_::html_close(stream);
        return stream;
    }

};

template <class DocumentT>
struct default_presenter<DocumentT, void>: basic_presenter<DocumentT>{
    using basic_presenter_ = basic_presenter<DocumentT>;

    default_presenter(const DocumentT& doc): basic_presenter_(doc) {}

    friend DocumentT;

    template <typename Stream>
    Stream& operator()(Stream& stream) const {
        basic_presenter_::html_open(stream);
            basic_presenter_::head(stream);
            basic_presenter_::body_open(stream);
                generate_body(stream);
            basic_presenter_::body_close(stream);
        basic_presenter_::html_close(stream);
        return stream;
    }

    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream, std::size_t i, std::size_t len) const {
        basic_presenter_::present(key, str, stream, i, len);
    }

    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream) const {
        basic_presenter_::present(key, str, stream);
    }

    private:
        template <typename StreamT>
        StreamT& generate_body(StreamT& stream) const {
            basic_presenter_::document().apply(*this, stream);
            return stream;
        }

};

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_PRESENTER_H
