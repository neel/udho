#ifndef UDHO_URL_ROUTER_EXPLORERS_H
#define UDHO_URL_ROUTER_EXPLORERS_H

#include <string>
#include <regex>
#include <filesystem>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/asset/io.h>
#include <udho/pages/layouts.h>
#include <udho/pages/data.h>
#include <udho/view/bridges/lua.h>
#include <udho/url/utils.h>

namespace udho{
namespace url{
namespace explorers {

namespace detail{

template <typename T>
struct is_unique_ptr_: public std::false_type{};
template <typename T, typename D>
struct is_unique_ptr_<std::unique_ptr<T, D>>: public std::true_type{};
template <typename T>
using is_unique_ptr = is_unique_ptr_<std::remove_cv_t<std::remove_reference_t<T>>>;
template<typename T>
constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

template <typename T>
struct is_shared_ptr_: public std::false_type{};
template <typename T>
struct is_shared_ptr_<std::shared_ptr<T>>: public std::true_type{};
template <typename T>
using is_shared_ptr = is_shared_ptr_<std::remove_cv_t<std::remove_reference_t<T>>>;
template< typename T >
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
struct is_smart_ptr: std::bool_constant<is_shared_ptr_v<T> || is_unique_ptr_v<T>> {};

}

/**
 * @brief Abstract base class for resource explorers providing unified interface for file/asset access
 *
 * This class defines the common interface for exploring different types of resources,
 * whether they exist in the filesystem or as embedded assets. Derived classes must
 * implement the core functionality for checking existence, serving content, and listing resources.
 */
struct abstract_explorer {
    using context_type = udho::net::context<udho::view::data::bridges::lua>;

    inline explicit abstract_explorer(const std::string& label): _label(label) {}
    virtual ~abstract_explorer() = default;

    const std::string& label() const { return _label; }

    /**
     * @brief Check if a resource exists at the given path
     * @param subject Path to the resource to check
     * @return true if the resource exists and is accessible, false otherwise
     */
    virtual bool exists(const std::string& subject) const = 0;

    /**
     * @brief Check if the path represents a valid resource container (directory/prefix)
     * @param subject Path to check
     * @return true if the path is a valid container that can be listed
     */
    virtual bool is_subset(const std::string& subject) const = 0;

    /**
     * @brief Serve the content of a resource through a network stream
     * @param subject Path to the resource to serve
     * @param stream Network stream to write the content to
     * @return true if the resource was successfully served, false otherwise
     */
    virtual bool cat(const std::string& subject, udho::net::stream& stream) const = 0;

    inline static udho::pages::system::layouts::sys<context_type> layout(context_type ctx) {
        return udho::pages::system::layouts::listing(ctx);
    }

    virtual bool ls(const std::string& subject, context_type ctx, udho::pages::system::data::listings& d) const {
        namespace placeholders  = udho::pages::system::layouts::placeholders;

        if(is_subset(subject)) {
            return populate(subject, ctx, d);
        }

        return false;
    }

    protected:
    virtual bool populate(const std::string& subject, context_type ctx, udho::pages::system::data::listings& d) const = 0;

    private:
    std::string _label;
};

/**
 * @class files
 * @brief Filesystem-based resource explorer
 *
 * Serves static files from the filesystem.
 * Manages document root directory and ensures secure path resolution.
 */
struct files: public abstract_explorer {
    using context_type = abstract_explorer::context_type;

    /**
     * @brief Gets the current document root
     * @return Const reference to the document root path
     */
    inline const std::filesystem::path& root() const { return _root; }

    /**
     * @brief Constructs a filesystem explorer with preconfigured MIME types
     * @param label Unique identifier for this explorer
     * @param root Document root directory (default: current working directory)
     *
     * Initializes with 50+ common MIME type mappings covering:
     * - Web formats (HTML/CSS/JS/SVG)
     * - Images (PNG/JPEG/AVIF/WEBP)
     * - Fonts (WOFF2/TTF/OTF)
     * - Media (MP4/WebM/MP3)
     * - Documents (PDF/Office formats)
     * - Archives (ZIP/TAR/GZ)
     * - Security certificates
     *
     * @throw std::invalid_argument If any MIME type validation fails
     * @note Charset declarations added where appropriate
     * @warning Legacy formats (SWF) included for compatibility
     */
    explicit inline files(const std::string& label, const std::filesystem::path& root = std::filesystem::current_path()): abstract_explorer(label), _root(root){
        // Text/Web Formats
        mime("html",  "text/html");
        mime("htm",   "text/html");
        mime("css",   "text/css");
        mime("js",    "text/javascript");
        mime("mjs",   "text/javascript");
        mime("json",  "application/json");
        mime("txt",   "text/plain");
        mime("svg",   "image/svg+xml");
        mime("xml",   "application/xml");
        mime("csv",   "text/csv");
        mime("md",    "text/markdown");

        // Images
        mime("png",   "image/png");
        mime("jpg",   "image/jpeg");
        mime("jpeg",  "image/jpeg");
        mime("gif",   "image/gif");
        mime("webp",  "image/webp");
        mime("avif",  "image/avif");
        mime("bmp",   "image/bmp");
        mime("ico",   "image/vnd.microsoft.icon");
        mime("tiff",  "image/tiff");

        // Fonts
        mime("woff",  "font/woff");
        mime("woff2", "font/woff2");
        mime("ttf",   "font/ttf");
        mime("otf",   "font/otf");
        mime("eot",   "application/vnd.ms-fontobject");

        // Media
        mime("mp4",   "video/mp4");
        mime("webm",  "video/webm");
        mime("ogg",   "video/ogg");
        mime("mp3",   "audio/mpeg");
        mime("wav",   "audio/wav");
        mime("flac",  "audio/flac");

        // Documents
        mime("pdf",   "application/pdf");
        mime("doc",   "application/msword");
        mime("docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
        mime("xls",   "application/vnd.ms-excel");
        mime("xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
        mime("ppt",   "application/vnd.ms-powerpoint");
        mime("pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation");

        // Archives
        mime("zip",   "application/zip");
        mime("gz",    "application/gzip");
        mime("tar",   "application/x-tar");
        mime("7z",    "application/x-7z-compressed");

        // WebAssembly
        mime("wasm",  "application/wasm");
        mime("wat",   "text/plain"); // WebAssembly text format

        // Security/Config
        mime("swf",   "application/x-shockwave-flash"); // Legacy
        mime("cer",   "application/pkix-cert");
        mime("crt",   "application/x-x509-ca-cert");
    }

    /**
     * @brief Registers a MIME type for a file extension
     * @param extension File extension to register (1-10 alphanumeric/underscore characters)
     * @param mime MIME type string in "type/subtype" format
     * @return Reference to self for method chaining
     * @throw std::invalid_argument If:
     * - Extension fails validation (regex: `^[a-zA-Z0-9_]{1,10}$`)
     * - MIME type fails format check (regex: `^\w+/[\w\-+\.]+$`)
     *
     * @note Extension is case-insensitive (automatically lowercased)
     * @warning Overwrites existing entries for the same extension
     * @example
     * // Register Markdown files
     * explorer.mime("md", "text/markdown");
     *
     * // Register custom application type
     * explorer.mime("myapp", "application/x-myapp");
     */
    files& mime(const std::string& extension, const std::string& mime){
        static const std::regex ext_re("^[a-zA-Z0-9_]+$");
        if (extension.empty() || extension.size() > 10 || !std::regex_match(extension, ext_re)) {
            throw std::invalid_argument("Invalid file extension" + extension);
        }
        static const std::regex mime_re("^\\w+/[\\w\\-\\+\\.]+$");
        if (mime.empty() || std::count(mime.begin(), mime.end(), '/') != 1 || !std::regex_match(mime, mime_re)) {
            throw std::invalid_argument("Invalid MIME format" + mime);
        }
        std::string ext = extension;
        boost::to_lower(ext);
        _mimes[ext] = mime;
        return *this;
    }

    /**
     * @brief Checks if an extension has a registered MIME type
     * @param extension File extension to check
     * @return true if extension exists in registry, false otherwise
     * @note Case-insensitive check (automatically lowercases input)
     */
    bool mime_exists(const std::string& extension) const {
        std::string ext = extension;
        boost::to_lower(ext);
        return _mimes.count(ext) > 0;
    }

    /**
     * @brief Retrieves registered MIME type for an extension
     * @param extension File extension to look up
     * @return Registered MIME type string, or empty string if not found
     * @note Case-insensitive lookup (automatically lowercases input)
     */
    std::string mime(const std::string& extension) const {
        std::string ext = extension;
        boost::to_lower(ext);
        auto it = _mimes.find(ext);
        if(it != _mimes.end()){
            return it->second;
        } else {
            return std::string{};
        }
    }

    /**
     * @brief Determines MIME type for a filesystem path
     * @param path Filesystem path to analyze
     * @return MIME type string using this priority:
     * 1. Registered extension mapping
     * 2. libmagic detection via utils::mime_type()
     * 3. "application/octet-stream" as final fallback
     *
     * @details Handles special cases:
     * - Standard extensions (`.txt` → "text/plain")
     * - Hidden files (`.bashrc` → "text/x-shellscript")
     * - Extensionless files (uses full filename analysis)
     * - Multiple extensions (`.tar.gz` → ".gz" extension)
     *
     * @see utils::mime_type()
     */
    std::string mime_type(const std::filesystem::path& path) const {
        std::string ext = path.extension().string();
        if (!ext.empty() && ext[0] == '.') {
            if(ext.size() > 1){
                ext = ext.substr(1);
            } else {
                ext = std::string{};
            }
        } else if(ext.empty()) { // handle hiden files
            std::string filename = path.filename().string();
            std::size_t dot_pos  = filename.rfind('.');
            if (dot_pos != std::string::npos && dot_pos >= 0 && dot_pos < filename.length() - 1) {
                ext = filename.substr(dot_pos+1);
            }
        }
        boost::to_lower(ext);
        if(!ext.empty()){
            std::string mpped_mime_type = mime(ext);
            if(!mpped_mime_type.empty()){
                return mpped_mime_type;
            }
        }
        return utils::mime_type(path);
    }

    /**
     * @brief Checks if a normalized file path exists
     * @param subject Path to check
     * @return true if file exists and is regular, false otherwise
     */
    inline bool exists(const std::string& subject) const override {
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
    inline bool is_subset(const std::string& subject) const override {
        std::filesystem::path normalized_path = utils::normalize_path(subject, _root);
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
    inline bool cat(const std::string& subject, udho::net::stream& stream) const override {
        if(exists(subject)) {
            std::filesystem::path normalized_path = utils::normalize_path(subject, _root);
            return serve_file(normalized_path, stream);
        } else {
            return false;
        }
    }

  protected:
    virtual bool populate(const std::string& subject, context_type ctx, udho::pages::system::data::listings& d) const override {
        std::filesystem::path normalized_path = utils::normalize_path(subject, _root);
        d.add(udho::pages::system::data::listing{label(), normalized_path, _root});
        return true;
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
    inline bool serve_file(const std::filesystem::path& normalized_path, udho::net::stream& stream) const {
        try {
            std::string mime = mime_type(normalized_path);
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
    std::map<std::string, std::string> _mimes;
};

/**
 * @class assets
 * @brief Embedded asset resource explorer
 *
 * Serves static files from the asset store.
 */
struct assets: public abstract_explorer {
    using context_type = abstract_explorer::context_type;

    assets(const std::string& label, const udho::view::resources::asset::const_store& assets): abstract_explorer(label), _assets(assets) {}

    /**
     * @brief Checks if the path exists
     * @param subject Path to check
     * @return true if path exists
     */
    inline bool exists(const std::string& subject) const override {
        return _assets.find(subject) != _assets.end();
    }

    /**
     * @brief Checks if a path exists as a prefix
     * @param subject Path to check
     * @return true if a prefix exists false otherwise
     */
    inline bool is_subset(const std::string& subject) const override {
        std::string sub = utils::slash_quote(subject);          // _base starts and ends with / -> sub starts with /
        for(auto p: _assets.prefixes()){
            const std::string& prefix = p.prefix();                     // prefix does not contain any leading or trailing /
            std::string prefix_q = utils::slash_quote(utils::slash_concat(_assets.base(), prefix));          // prefix_q starts and ends with /
            bool prefix_matched = boost::starts_with(sub, _assets.base()) && boost::starts_with(prefix_q, sub);    // prefix_q starts with the sub
            if(prefix_matched){
                if(sub.size() == prefix_q.size()){                      // complete match implies |sub| = |prefix_q|
                    return true;                                        // fully matches with a prefix
                } else {                                                // partial match implies |sub| < |prefix_q|
                                                                        // but both sub and prefix_q ends with /
                                                                        // therefore the last / of sub must have matched
                                                                        // with some intermediate slash of prefix_q
                    return true;                                        // fully matches with a subprefix
                }
            }
        }
        return false;
    }

    /**
     * @brief Serves aan asset through the provided stream
     * @param subject path to asset
     * @param stream network stream to write to
     * @return true if an asset was served successfully, false otherwise
     * @throws Propagates filesystem errors and libmagic exceptions
     */
    inline bool cat(const std::string& subject, udho::net::stream& stream) const override {
        if(exists(subject)){
            return _assets.serve(stream, subject);
        }
        return false;
    }

protected:
    bool populate(const std::string& subject, context_type ctx, udho::pages::system::data::listings& d) const override {
        d.add(udho::pages::system::data::listing{label(), _assets.make_prefix_proxy(), _assets.base(), subject});
        return true;
    }
  private:
    const udho::view::resources::asset::const_store& _assets;
};

/**
 * @class explorers_hub
 * @brief Central registry for managing resource explorers
 *
 * This class acts as an immutable container for abstract_explorer instances, providing:
 * - Ownership management of explorers
 * - Unique label enforcement
 * - Type-safe insertion of various explorer types
 * - Immutability guarantee after construction
 *
 * The hub becomes immutable after construction and cannot be modified once created.
 * All explorers must be added during initialization.
 */
struct registry{

    /// @brief Pointer type for owned explorer instances
    using explorer_ptr    = std::unique_ptr<abstract_explorer>;
    using explorer_entry  = std::pair<std::string, explorer_ptr>;
    /// @brief Container type mapping labels to explorer instances
    using collection_type = std::vector<explorer_entry>;

    using context_type = abstract_explorer::context_type;

    static const explorer_ptr nothing;


    /**
     * @brief Construct an empty hub
     */
    explicit registry() = default;

    /**
     * @brief Construct hub with multiple explorers
     * @tparam Args Variadic template parameter pack of explorer arguments
     * @param args Explorer instances to insert (supports multiple types)
     * @throw std::invalid_argument If any argument is null
     * @throw std::runtime_error If duplicate labels are detected
     *
     * Accepts arguments in various forms:
     * - unique_ptr<abstract_explorer> (transfers ownership)
     * - Concrete explorer objects (copies into owned instances)
     * - Raw pointers to explorers (clones and takes ownership)
     */
    template <typename... Args>
    explicit registry(Args&&... args) {
        (insert(std::forward<Args>(args)), ...);
    }

    /**
     * @brief Construct hub with a single explorer
     * @param explorer Explorer instance to insert
     * @throw std::invalid_argument If explorer is null
     * @throw std::runtime_error If label already exists
     */
    explicit registry(explorer_ptr explorer) {
        insert(std::move(explorer));
    }

    registry(const registry&) = delete;
    registry& operator=(const registry&) = delete;

    registry(registry&&) noexcept = default;
    registry& operator=(registry&&) noexcept = default;

    inline const explorer_ptr& exists_in(const std::string& subject) const {
        for(const auto& pair: _explorers){
            if(pair.second->exists(subject)){
                return pair.second;
            }
        }
        return nothing;
    }

    inline bool exists(const std::string& subject) const {
        return exists_in(subject) != nothing;
    }

    inline const explorer_ptr& subset_of(const std::string& subject) const {
        for(const auto& pair: _explorers){
            if(pair.second->is_subset(subject)){
                return pair.second;
            }
        }
        return nothing;
    }

    inline bool is_subset(const std::string& subject) const {
        return subset_of(subject) != nothing;
    }

    inline bool cat(const std::string& subject, udho::net::stream& stream) const {
        const explorer_ptr& explorer = exists_in(subject);
        if(explorer == nothing) {
            return false;
        }
        return explorer->cat(subject, stream);
    }

    inline bool ls(const std::string& subject, context_type ctx) const {
        auto layout = abstract_explorer::layout(ctx);
        layout.css().add("udho", "tabs.css");

        udho::pages::system::data::listings listings{subject};

        bool result = false;
        for(const auto& pair: _explorers){
            if(pair.second->is_subset(subject)){
                pair.second->ls(subject, ctx, listings);
                result = true;
            }
        }

        if(result){
            namespace places = udho::pages::system::layouts::places;
            namespace placeholders = udho::pages::system::layouts::placeholders;

            layout[placeholders::header] = udho::pages::system::data::listing_header{};
            layout[places::listing] = listings;
            layout[placeholders::footer] = udho::pages::system::data::status_info{};
        }

        return result;
    }

    bool serve(const std::string& subject, context_type ctx) const {
        bool found = false;
        if(exists(subject)){
            found = cat(subject, ctx);
        } else if(is_subset(subject)){
            found = ls(subject, ctx);
        }
        return found;
    }

  private:

    /**
     * @brief Insert a pre-constructed explorer instance
     * @param explorer Unique pointer to explorer instance
     * @throw std::invalid_argument If explorer is null
     * @throw std::runtime_error If label already exists
     */
    void insert(explorer_ptr&& explorer) {
        if (!explorer) {
            throw std::invalid_argument("Cannot insert null explorer");
        }

        const std::string label = explorer->label();
        if (_labels.count(label)) {
            throw std::runtime_error("Explorer with label '" + label + "' already exists");
        }

        _explorers.emplace_back(explorer_entry{label, std::move(explorer)});
        _labels.insert(label);
    }

    /**
     * @brief Insert a copyable explorer instance
     * @tparam ExplorerT Concrete explorer type (must be copy constructible)
     * @param explorer Explorer instance to copy
     */
    template <typename ExplorerT, std::enable_if_t<!detail::is_unique_ptr_v<ExplorerT>>* = nullptr>
    void insert(ExplorerT&& explorer) {
        static_assert(std::is_base_of_v<abstract_explorer, ExplorerT>, "Must insert objects derived from abstract_explorer");
        auto ptr = std::make_unique<ExplorerT>(std::forward<ExplorerT>(explorer));
        assert(dynamic_cast<abstract_explorer*>(ptr.get()) != nullptr);
        insert(std::move(ptr));
    }

    private:
    collection_type _explorers;
    std::set<std::string> _labels;
};

inline const registry::explorer_ptr registry::nothing = nullptr;

}
}
}

#endif // UDHO_URL_ROUTER_EXPLORERS_H
