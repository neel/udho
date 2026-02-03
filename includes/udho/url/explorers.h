#ifndef UDHO_URL_ROUTER_EXPLORERS_H
#define UDHO_URL_ROUTER_EXPLORERS_H

#include <string>
#include <filesystem>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/asset/io.h>
#include <udho/pages/layouts.h>
#include <udho/pages/data.h>
#include <udho/view/bridges/lua.h>
#include <udho/url/utils.h>
#include <udho/url/mimes.h>
#include <udho/net/ostream.h>

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
    virtual bool cat(const std::string& subject, udho::net::ostream_view& stream) const = 0;

    template <typename ContextT>
    inline static udho::pages::system::layouts::sys<ContextT> layout(ContextT& context) {
        return udho::pages::system::layouts::listing(context);
    }

    bool _ls(const std::string& subject, udho::pages::system::data::listings& d) const {
        namespace placeholders  = udho::pages::system::layouts::placeholders;

        if(is_subset(subject)) {
            return populate(subject, d);
        }

        return false;
    }

    virtual bool ls(const std::string& subject, udho::pages::system::data::listings& d) const {
        return _ls(subject, d);
    }

    protected:
    virtual bool populate(const std::string& subject, udho::pages::system::data::listings& d) const = 0;

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
struct files: public abstract_explorer, public mime_registry {
    /**
     * @brief Gets the current document root
     * @return Const reference to the document root path
     */
    inline const std::filesystem::path& root() const { return _root; }

    /**
     * @brief Constructs a filesystem explorer with preconfigured MIME types
     * @param label Unique identifier for this explorer
     * @param root Document root directory (default: current working directory)
     */
    explicit inline files(const std::string& label, const std::filesystem::path& root = std::filesystem::current_path()): abstract_explorer(label), _root(root) {}

    const mime_registry& mimes() const { return *this; }

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
    inline bool cat(const std::string& subject, udho::net::ostream_view& ostream) const override {
        if(exists(subject)) {
            std::filesystem::path normalized_path = utils::normalize_path(subject, _root);
            return serve_file(normalized_path, ostream);
        } else {
            return false;
        }
    }

  protected:
    virtual bool populate(const std::string& subject, udho::pages::system::data::listings& d) const override {
        std::filesystem::path normalized_path = utils::normalize_path(subject, _root);
        d.add(udho::pages::system::data::listing{label(), normalized_path, _root, *this});
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

    template <typename OStreamT>
    inline bool serve_file(const std::filesystem::path& normalized_path, OStreamT& ostream) const {
        try {
            std::string mime = mime_type(normalized_path);
            boost::iostreams::mapped_file_source file;
            file.open(normalized_path);

            if (file.is_open()) {
                ostream.set(boost::beast::http::field::content_type, mime);
                ostream.set(boost::beast::http::field::content_length, std::to_string(file.size()));

                ostream.write(file.data(), file.size());
                file.close();
                ostream.finish();
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

/**
 * @class assets
 * @brief Embedded asset resource explorer
 *
 * Serves static files from the asset store.
 */
struct assets: public abstract_explorer {
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
        for(const auto& p: _assets.prefixes()){
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
    inline bool cat(const std::string& subject, udho::net::ostream_view& ostream) const override {
        if(exists(subject)){
            return _assets.serve(ostream, subject);
        }
        return false;
    }

protected:
    bool populate(const std::string& subject, udho::pages::system::data::listings& d) const override {
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

    enum class status {
        unknown,
        file,
        directory,
        not_found
    };

    /// @brief Pointer type for owned explorer instances
    using explorer_ptr    = std::unique_ptr<abstract_explorer>;
    using explorer_entry  = std::pair<std::string, explorer_ptr>;
    /// @brief Container type mapping labels to explorer instances
    using collection_type = std::vector<explorer_entry>;

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

    template <typename OstreamT>
    inline bool cat(const std::string& subject, OstreamT& stream) const {
        const explorer_ptr& explorer = exists_in(subject);
        if(explorer == nothing) {
            return false;
        }
        return explorer->cat(subject, stream);
    }

    template <typename ContextT>
    inline bool ls(const std::string& subject, ContextT& context) const {
        auto layout = abstract_explorer::layout(context);
        layout.css().add("udho", "tabs.css");

        udho::pages::system::data::listings listings{subject};

        bool result = false;
        for(const auto& pair: _explorers){
            if(pair.second->is_subset(subject)){
                pair.second->ls(subject, listings);
                result = true;
            }
        }

        if(result){
            namespace places = udho::pages::system::layouts::places;
            namespace placeholders = udho::pages::system::layouts::placeholders;

            layout[placeholders::header] = udho::pages::system::data::listing_header{boost::beast::http::status::ok};
            layout[places::listing] = listings;
            layout[placeholders::footer] = udho::pages::system::data::status_info{};
        }

        return result;
    }

    template <typename ContextT>
    status serve(const std::string& subject, ContextT& context) const {
        if(exists(subject)){
            udho::net::ostream_view ostream_view = context.ostream().view();
            if(cat(subject, ostream_view))
                return status::file;
        } else if(is_subset(subject)){
            if(ls(subject, context))
                return status::directory;
        }
        return status::not_found;
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
