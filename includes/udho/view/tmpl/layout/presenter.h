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
 *
 * @tparam DocumentT Document type from which presentation state is obtained.
 *
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

    /**
     * @brief Construct a presenter bound to a document.
     *
     * @param document Document whose contents and presentation metadata will be rendered.
     *
     * @warning The document must outlive the presenter.
     */
    basic_presenter(const document_type& document): _document(document) {}
    private:
        const document_type& _document;
    public:
        /**
         * @brief Access the document associated with this presenter.
         * @return Read-only reference to the document.
         */
        const document_type& document() const { return _document; }
    protected:

        /**
         * @brief Write the document declaration and opening `<html>` tag.
         *
         * Emits the doctype when enabled and writes any configured language,
         * namespace, direction, and class attributes.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
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

        /**
         * @brief Write the closing `</html>` tag.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
        template <typename StreamT>
        StreamT& html_close(StreamT& stream) const {
            stream << "</html>";
            return stream;
        }

        /**
         * @brief Write the opening document body tag.
         *
         * The tag and its attributes are obtained from the document's body configuration.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
        template <typename StreamT>
        StreamT& body_open(StreamT& stream) const {
            stream << _document.body().open();
            return stream;
        }

        /**
         * @brief Write embedded JavaScript assets and close the document body.
         *
         * Embedded JavaScript is emitted after the body contents and immediately before the closing body tag.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
        template <typename StreamT>
        StreamT& body_close(StreamT& stream) const {
            _document.js().write_embedded(stream);
            stream << _document.body().close();
            return stream;
        }

        /**
         * @brief Write the document title element.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
        template <typename StreamT>
        StreamT& title(StreamT& stream) const {
            stream << "<title>" << _document.preamble().title() << "</title>";
            return stream;
        }

        /**
         * @brief Write all metadata from the document preamble.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
        template <typename StreamT>
        StreamT& meta(StreamT& stream) const {
            _document.preamble().meta.write(stream);
            return stream;
        }

        /**
         * @brief Write assets placed in the document head.
         *
         * Assets are written in the following order:
         *
         * 1. JavaScript import map;
         * 2. externally referenced JavaScript;
         * 3. externally referenced CSS;
         * 4. embedded CSS.
         *
         * Embedded JavaScript is intentionally not emitted here; it is emitted by
         * body_close().
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
        template <typename StreamT>
        StreamT& includes(StreamT& stream) const {
            _document.js().importmap(stream);
            _document.js().write(stream);
            _document.css().write(stream);
            _document.css().write_embedded(stream);
            return stream;
        }

        /**
         * @brief Write assets placed in the document head.
         *
         * Assets are written in the following order:
         *
         * 1. JavaScript import map;
         * 2. externally referenced JavaScript;
         * 3. externally referenced CSS;
         * 4. embedded CSS.
         *
         * Embedded JavaScript is intentionally not emitted here; it is emitted by
         * body_close().
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
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
        /**
         * @brief Present every value associated with a multi-valued placeholder.
         *
         * Each value is presented in its stored order. Placeholder properties are
         * applied around the complete sequence rather than separately around each
         * value.
         *
         * @tparam KeyT Placeholder key type.
         * @tparam Stream Output stream type.
         * @param key Placeholder key.
         * @param stream Destination stream.
         */
        template <typename KeyT, typename Stream>
        void present_all(const KeyT& key, Stream& stream) const {
            std::size_t len = _document[key].count();
            if(len > 0){
                for(std::size_t i = 0; i != len; ++i) {
                    present(key, *_document[key], stream, i, len);
                }
            }
        }

        /**
         * @brief Present a single-valued placeholder when it contains a value.
         *
         * No output is produced when the placeholder does not exist.
         *
         * @tparam KeyT Placeholder key type.
         * @tparam Stream Output stream type.
         * @param key Placeholder key.
         * @param stream Destination stream.
         */
        template <typename KeyT, typename Stream>
        void present(const KeyT& key, Stream& stream) const {
            if(_document[key].exists())
                present(key, *_document[key], stream);
        }

    protected:
        /**
         * @brief Present one placeholder value.
         *
         * If presentation properties are configured for the placeholder, their
         * opening and closing tags are written around the value.
         *
         * @tparam KeyT Placeholder key type.
         * @tparam Stream Output stream type.
         * @param key Placeholder key.
         * @param str Rendered placeholder contents.
         * @param stream Destination stream.
         */
        template <typename KeyT, typename Stream>
        void present(const KeyT& key, const std::string& str, Stream& stream) const {
            const auto& properties = document().properties(key);
            bool tag_opened = false;
            if(properties.isset()){
                stream << properties.open();
                tag_opened = true;
            }

            stream.write(str.c_str(), str.size(), true);
            if(tag_opened){
                stream << properties.close();
            }
        }

        /**
         * @brief Present one element of a multi-valued placeholder.
         *
         * The placeholder's opening tag is written before the first value and its
         * closing tag is written after the final value. This causes all values to
         * share one enclosing element.
         *
         * @tparam KeyT Placeholder key type.
         * @tparam Stream Output stream type.
         * @param key Placeholder key.
         * @param str Current rendered value.
         * @param stream Destination stream.
         * @param i Zero-based index of the current value.
         * @param len Total number of values for this placeholder.
         */
        template <typename KeyT, typename Stream>
        void present(const KeyT& key, const std::string& str, Stream& stream, std::size_t i, std::size_t len) const {
            const auto& properties = document().properties(key);

            if(i == 0 && properties.isset()){
                stream << properties.open();
            }

            stream.write(str.c_str(), str.size(), true);

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

/**
 * @brief Default presenter with a custom CRTP body renderer.
 *
 * The common HTML envelope, head, asset placement, body tags, and closing
 * elements are produced by basic_presenter. The contents of the body are
 * delegated to `Derived::render(stream)`.
 *
 * @tparam DocumentT Document type being presented.
 * @tparam Derived CRTP-derived presenter providing `render(Stream&) const`.
 */
template <class DocumentT, class Derived>
struct default_presenter: basic_presenter<DocumentT>{
    using basic_presenter_ = basic_presenter<DocumentT>;

    /**
     * @brief Construct a custom-body presenter for a document.
     * @param doc Document to present.
     *
     * @warning `doc` must outlive this presenter.
     */
    default_presenter(const DocumentT& doc): basic_presenter_(doc) {}

    friend DocumentT;

    /**
     * @brief Present the complete document.
     *
     * Delegates body-content generation to `Derived::render(stream)` while the
     * base presenter writes the common HTML structure and assets.
     *
     * @tparam Stream Output stream type.
     * @param stream Destination stream.
     * @return Reference to `stream`.
     */
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

/**
 * @brief Default presenter that generates the body from document placeholders.
 *
 * Placeholder values are emitted in the ordering defined by the document's
 * placeholder container. Configured placeholder properties are applied while
 * each single- or multi-valued placeholder is presented.
 *
 * @tparam DocumentT Document type being presented.
 */
template <class DocumentT>
struct default_presenter<DocumentT, void>: basic_presenter<DocumentT>{
    using basic_presenter_ = basic_presenter<DocumentT>;

    /**
     * @brief Construct a default placeholder-based presenter.
     * @param doc Document to present.
     *
     * @warning `doc` must outlive this presenter.
     */
    default_presenter(const DocumentT& doc): basic_presenter_(doc) {}

    friend DocumentT;

    /**
     * @brief Present the complete document.
     *
     * Writes the common HTML structure and generates the body by applying this
     * presenter to the document's placeholders.
     *
     * @tparam Stream Output stream type.
     * @param stream Destination stream.
     * @return Reference to `stream`.
     */
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

    /**
     * @brief Placeholder callback for a value in a multi-valued placeholder.
     *
     * This overload is invoked by the placeholder container while applying the
     * presenter.
     *
     * @tparam KeyT Placeholder key type.
     * @tparam Stream Output stream type.
     * @param key Placeholder key.
     * @param str Current placeholder value.
     * @param stream Destination stream.
     * @param i Zero-based value index.
     * @param len Total number of values.
     */
    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream, std::size_t i, std::size_t len) const {
        basic_presenter_::present(key, str, stream, i, len);
    }

    /**
     * @brief Placeholder callback for a single-valued placeholder.
     *
     * This overload is invoked by the placeholder container while applying the
     * presenter.
     *
     * @tparam KeyT Placeholder key type.
     * @tparam Stream Output stream type.
     * @param key Placeholder key.
     * @param str Placeholder value.
     * @param stream Destination stream.
     */
    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream) const {
        basic_presenter_::present(key, str, stream);
    }

    private:
        /**
         * @brief Generate body contents by applying this presenter to the document.
         *
         * @tparam StreamT Output stream type.
         * @param stream Destination stream.
         * @return Reference to `stream`.
         */
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
