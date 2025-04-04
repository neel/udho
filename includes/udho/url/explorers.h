#ifndef UDHO_URL_ROUTER_EXPLORERS_H
#define UDHO_URL_ROUTER_EXPLORERS_H

#include <string>
#include <filesystem>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/asset/io.h>
#include <udho/pages/layouts.h>
#include <udho/pages/data.h>
#include <udho/view/bridges/lua.h>

namespace udho{
namespace url{

namespace utils{

template <typename Ch>
inline std::filesystem::path normalize_path(const std::basic_string<Ch>& subject, const std::filesystem::path& root = std::filesystem::current_path()) {
    std::string relative_subject = subject;
    if (!relative_subject.empty() && relative_subject[0] == '/') {
        relative_subject.erase(0, 1); // Remove the leading slash if present
    }

    std::filesystem::path requested_path = root / relative_subject;
    std::filesystem::path normalized_path;
    try {
        normalized_path = std::filesystem::weakly_canonical(requested_path);
        if (!boost::algorithm::starts_with(normalized_path.string(), root.string())) {
            std::cout << "Security alert: Attempted access outside of the document root. " << normalized_path << " " << root << std::endl;
            return std::filesystem::path{};
        }
    } catch(const std::filesystem::filesystem_error& e) {
        std::cout << "Filesystem error: " << e.what() << std::endl;
        return std::filesystem::path{};
    }
    return normalized_path;
}

/**
 * @brief Determines MIME type of a file using libmagic
 * @param path Filesystem path to analyze
 * @return MIME type as string
 * @note Requires libmagic development files during compilation
 */
inline std::string mime_type(const std::filesystem::path& path) {
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, nullptr);
    const char* mime_type = magic_file(magic, path.c_str());
    std::string result = mime_type ? mime_type : "application/octet-stream";
    magic_close(magic);
    return result;
}

}

struct files_explorer{
    using context_type = udho::net::context<udho::view::data::bridges::lua>;

    explicit inline files_explorer(const std::filesystem::path& root = std::filesystem::current_path()): _root(root){}

    /**
     * @brief Gets the current document root
     * @return Const reference to the document root path
     */
    inline const std::filesystem::path& root() const {
        return _root;
    }
    /**
     * @brief Checks if a normalized file path exists
     * @param subject Path to check
     * @return true if file exists and is regular, false otherwise
     */
    inline bool exists(const std::string& subject) const {
        std::filesystem::path normalized_path = normalize(subject);
        return
            !normalized_path.empty()
            && std::filesystem::exists(normalized_path)
            && std::filesystem::is_regular_file(normalized_path);
    }
    /**
     * @brief Checks if a normalized file path exists as a directory inside root
     * @param subject Path to check
     * @return true if directory exists false otherwise
     */
    inline bool is_subset(const std::string& subject) const {
        std::filesystem::path normalized_path = utils::normalize_path(subject);
        return
                  !normalized_path.empty()
               && std::filesystem::exists(normalized_path)
               && std::filesystem::is_directory(normalized_path);
    }
    /**
     * @brief Serves a file through the provided stream
     * @param subject Path to serve
     * @param stream Network stream to write to
     * @return true if file was served successfully, false otherwise
     * @throws Propagates filesystem errors and libmagic exceptions
     */
    inline bool cat(const std::string& subject, udho::net::stream& stream) const {
        if(exists(subject)) {
            std::filesystem::path normalized_path = utils::normalize_path(subject);
            return serve_file(normalized_path, stream);
        } else {
            return false;
        }
    }
    /**
     * @brief serves a directory or file from docroot
     * @param subject Path to serve
     * @param ctx context
     * @return boolean value indicating success
     */
    bool ls(const std::string& subject, context_type ctx) const {
        if(is_subset(subject)) {
            std::filesystem::path normalized_path = utils::normalize_path(subject);
            auto layout     = udho::pages::system::layouts::listing(ctx);
            ls(subject, ctx, layout);
            return true;
        } else {
            return false;
        }
    }
    /**
     * @brief serves a directory or file from docroot by populating an existing layout
     * @param subject Path to serve
     * @param ctx context
     * @param layout layout to populate
     * @return the same layout (populated only is the subject is_subset of the root)
     */
    template <typename ContextT>
    udho::pages::system::layouts::sys<ContextT>& ls(const std::string& subject, ContextT ctx, udho::pages::system::layouts::sys<ContextT>& layout) const {
        namespace placeholders  = udho::pages::system::layouts::placeholders;
        namespace places        = udho::pages::system::layouts::places;

        if(is_subset(subject)) {
            std::filesystem::path normalized_path = utils::normalize_path(subject);
            if(!layout[placeholders::header].exists()){
                layout[placeholders::header] = udho::pages::system::data::listing_header{subject, _root};
            }
            populate(normalized_path, ctx, layout);
            if(!layout[placeholders::header].exists()){
                layout[placeholders::footer] = udho::pages::system::data::status_info{};
            }
        }

        return layout;
    }
  private:
    template <typename ContextT>
    udho::pages::system::layouts::sys<ContextT>& populate(const std::string& subject, ContextT ctx, udho::pages::system::layouts::sys<ContextT>& layout) const {
        namespace places        = udho::pages::system::layouts::places;
        layout[places::files]   = udho::pages::system::data::directory_listing{subject, _root};
        return layout;
    }

    /**
     * @brief Normalizes and secures a filesystem path
     * @param subject Path to normalize
     * @return Normalized path or empty path if security check fails
     * @note Prevents directory traversal attacks by ensuring path stays within docroot
     */
    inline std::filesystem::path normalize(const std::string& subject) const {
        return utils::normalize_path(subject, _root);
    }
    static inline bool serve_file(const std::filesystem::path& normalized_path, udho::net::stream& stream) {
        try {
            std::string mime = utils::mime_type(normalized_path);
            boost::iostreams::mapped_file_source file;
            file.open(normalized_path);

            if (file.is_open()) {
                stream.set(boost::beast::http::field::content_type, mime);
                stream.set(boost::beast::http::field::content_length, std::to_string(file.size()));

                stream.write(file.data(), file.size());
                file.close();
                stream.finish();
                return true;
            } else {
                std::cout << "Failed to open file: " << normalized_path << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error serving file: " << e.what() << std::endl;
        }

        return false;
    }

  private:
    std::filesystem::path      _root;
};

struct asset_explorer{
    asset_explorer(const udho::view::resources::asset::const_store& assets, const std::string base): _assets(assets), _base(base) {}

    inline bool exists(const std::string& subject) const {
        return _assets.find(_base+subject) != _assets.end();
    }
    inline bool is_subset(const std::string& subject) const {
        std::string sub = _base+subject;
        for(auto p: _assets.prefixes()){
            const std::string& prefix = p.prefix();
            auto it = std::find_first_of(p.begin(), p.end(), sub.begin(), sub.end());
            if(it != p.end()){
                auto sub_it = sub.begin();
                std::advance(sub_it, prefix.size());
                if(sub_it == sub.end()){
                    return true;
                } else {
                    return *sub_it == '/' || *(sub_it+1) == '/';
                }
            }
        }
        return false;
    }
    inline bool cat(const std::string& subject, udho::net::stream& stream) const {
        if(exists(subject)){
            std::string sub = _base+subject;
            return _assets.serve(stream, subject);
        }
        return false;
    }
    template <typename ContextT>
    bool ls(const std::string& subject, ContextT ctx) const {
        std::string sub = _base+subject;
        for(auto p: _assets.prefixes()){
            const std::string& prefix = p.prefix();
            auto it = std::find_first_of(p.begin(), p.end(), sub.begin(), sub.end());
            if(it != p.end()){
                auto sub_it = sub.begin();
                std::advance(sub_it, prefix.size());
                if(sub_it == sub.end()){
                    // TODO inside a prefix
                    // List all assets inside it
                } else {
                    if(*sub_it == '/' || *(sub_it+1) == '/'){
                        // partially inside a prefix
                        // List the rest of the sub as a subprefix
                    }
                }
            }
        }
        return false;
    }

  private:
    template <typename ContextT>
    udho::pages::system::layouts::sys<ContextT>& populate(const std::string& subject, ContextT ctx, udho::pages::system::layouts::sys<ContextT>& layout) const {

    }
  private:
    const udho::view::resources::asset::const_store& _assets;
      std::string _base;
};

}
}

#endif // UDHO_URL_ROUTER_EXPLORERS_H
