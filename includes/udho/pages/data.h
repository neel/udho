#ifndef UDHO_PAGES_DATA_H
#define UDHO_PAGES_DATA_H

#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include <string>
#include <sstream>
#include <filesystem>
#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/asset/info.h>
#include <udho/view/resources/asset/const_store.h>
#include <udho/url/utils.h>
#include <udho/view/resources/asset/utils.h>
#include <udho/url/mimes.h>
#include <boost/beast/http.hpp>

#include <fstream>
#ifdef _WIN32
#include <windows.h>
#include <winternl.h>
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/utsname.h>
#endif

namespace udho{
namespace pages{
namespace system{
namespace data{

/**
 * @brief Describes one filesystem or registered-asset entry.
 * @ingroup DoxyG_pages
 */
class entry{
    std::string _name;
    bool        _is_directory;
    std::string _mime;
    std::size_t _size;
    std::string _url;
    std::string _type;

    public:
    /**
     * @brief Constructs an entry from a filesystem directory entry.
     * @param entry Filesystem entry.
     * @param root Root used to form the URL.
     * @param mimes MIME registry used for files.
     */
    inline explicit entry(const std::filesystem::directory_entry& entry, const std::filesystem::path& root, const udho::url::mime_registry& mimes):
        _name(entry.path().filename()), _is_directory(entry.is_directory()), _size(_is_directory ? 0 : entry.file_size())
    {
        // { type
        switch (entry.status().type()) {
            case std::filesystem::file_type::regular:    _type = "File";             break;
            case std::filesystem::file_type::directory:  _type = "Directory";        break;
            case std::filesystem::file_type::symlink:    _type = "Symlink";          break;
            case std::filesystem::file_type::block:      _type = "Block Device";     break;
            case std::filesystem::file_type::character:  _type = "Character Device"; break;
            case std::filesystem::file_type::fifo:       _type = "FIFO";             break;
            case std::filesystem::file_type::socket:     _type = "Socket";           break;
            default:                                     _type = "Unknown";          break;
        }
        // }

        // { url
        std::filesystem::path relative = std::filesystem::relative(entry.path(), root);
        std::string url_path = relative.string();
        std::replace(url_path.begin(), url_path.end(), '\\', '/');
        if (url_path.empty()) {
            _url = "/";
        } else{
            _url = (url_path.front() == '/') ? url_path : "/" + url_path;
        }
        if(_is_directory && _url.back() != '/'){
            _url.push_back('/');
        }
        // }

        if(!_is_directory){
            _mime = mimes.mime_type(entry.path());
        } else {
            _mime = "N/A";
        }
    }

    /**
     * @brief Constructs an entry from registered asset information.
     * @param info Registered asset information.
     * @param base Base URL.
     */
    inline explicit entry(const udho::view::resources::asset::asset_registration_info& info, const std::string& base):
        _name(info.name()), _is_directory(false), _mime(info.mime()), _size(0), _url(udho::url::utils::slash_concat(udho::url::utils::slash_concat(base, info.prefix()), info.name()))
    {
        _type = udho::view::resources::asset::utils::to_string(info.source()) + " " + udho::view::resources::asset::utils::to_string(info.type());
        if(info.owned()){
            _type = "owned " + _type;
        }
    }

    /**
     * @brief Constructs a directory entry for an asset prefix.
     * @param subprefix Asset subprefix.
     * @param base Base URL.
     */
    inline explicit entry(const std::string& subprefix, const std::string& base):
        _name(subprefix), _is_directory(true), _mime("N/A"), _size(0), _url(udho::url::utils::slash_quote(udho::url::utils::slash_concat(base, subprefix))), _type("prefix")
    {}

    /** @brief Returns the entry name. */
    inline const std::string& name() const { return _name; }
    /** @brief Returns the MIME type. */
    inline const std::string& mime() const { return _mime; }
    /** @brief Returns the entry URL. */
    inline const std::string& url() const { return _url; }
    /** @brief Returns the entry type description. */
    inline const std::string& type() const { return _type; }
    /** @brief Returns whether the entry represents a directory. */
    inline bool is_directory() const { return _is_directory; }
    /** @brief Returns the filename extension, or an empty string when absent. */
    inline std::string extension() const {
        if(_is_directory) return "";

        std::size_t dot_pos = _name.rfind('.');
        if (dot_pos != std::string::npos && dot_pos > 0 && dot_pos < _name.length() - 1) {
            return _name.substr(dot_pos+1);
        }
        return "";
    }
    /** @brief Returns a formatted size or `N/A` for directories. */
    inline std::string size() const {
        if (_is_directory) return "N/A";

        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        double size = static_cast<double>(_size);
        int unit = 0;

        while (size >= 1024 && unit < 4) {
            size /= 1024;
            unit++;
        }

        std::ostringstream oss;
        double intpart;
        if (std::modf(size, &intpart) == 0.0) {
            oss << static_cast<int>(intpart);
        } else {
            oss << std::fixed << std::setprecision(2) << size;
        }

        oss << ' ' << units[unit];
        return oss.str();
    }

    /** @brief Defines view-data metadata for an entry. */
    friend auto metatype(udho::view::data::type<entry>){
        using namespace udho::view::data;

        return assoc("entry"),
               fvar("name",        &entry::name),
               fvar("is_dir",      &entry::is_directory),
               fvar("mime",        &entry::mime),
               fvar("size",        &entry::size),
               fvar("url",         &entry::url),
               fvar("extension",   &entry::extension),
               fvar("type",        &entry::type);
    }
};

/**
 * @brief Collection of entries for one filesystem path or asset prefix.
 * @ingroup DoxyG_pages
 */
class listing{
    std::string         _base;
    std::vector<entry>  _entries;
    std::string         _subject;
    std::string         _label;

    public:
        using container_type = std::vector<entry>;
        using const_iterator = typename container_type::const_iterator;
        using value_type     = typename container_type::value_type;
        using size_type      = typename container_type::size_type;

        /**
         * @brief Constructs a listing from a filesystem directory.
         * @param label Listing label.
         * @param path Directory to enumerate.
         * @param root Root used to form entry URLs.
         * @param mimes MIME registry used for files.
         */
        inline listing(const std::string& label, const std::filesystem::path& path, const std::filesystem::path& root, const udho::url::mime_registry& mimes): _subject(path), _base(root), _label(label) {
            std::filesystem::directory_iterator dit{path};
            for(const std::filesystem::directory_entry& e: dit){
                _entries.emplace_back( entry{e, root, mimes} );
            }
        }
        /**
         * @brief Constructs a listing from an asset-store prefix proxy.
         * @param label Listing label.
         * @param proxy Asset prefix proxy.
         * @param base Base URL.
         * @param subject Subject prefix to list.
         */
        inline listing(const std::string& label, const udho::view::resources::asset::const_store::prefix_proxy& proxy, const std::string& base, const std::string& subject): _base(base), _subject(subject), _label(label) {
            std::string subject_q = udho::url::utils::slash_quote(_subject);
            for(const auto& group: proxy){
                std::string prefix = udho::url::utils::slash_quote(udho::url::utils::slash_concat(_base, group.prefix()));
                std::string::size_type pos = prefix.find(subject_q);
                if(pos == std::string::npos){                             // subject does not match with prefix;
                    continue;                                             //  skip this prefix and check the next
                } else if(pos > 0 ) {                                     // subject matches with prefix but the prefix does not start with the subject
                    continue;                                             //  skip this prefix and check the next
                } else {                                                  // subject starts with the prefix
                    if(subject_q.size() == prefix.size()){                // subject fully matches with the prefix
                        for(const auto& asset: group){                    // List all assets inside this prefix
                            entry entry{asset, base};
                            _entries.push_back(entry);
                        }
                    } else {                                                // subject partially matches with the prefix
                        std::string rest = prefix.substr(subject_q.size()); // List all sub-prefixes inside the prefix part
                        entry entry{rest, base};
                        _entries.push_back(entry);
                    }
                }
            }
        }

	/** @brief Returns the label. */
        std::string label() const { return _label; }

        /** @brief Returns the listed subject. */
        inline std::string subject() const { return _subject; }
        /** @brief Returns the number of entries. */
        inline std::size_t size() const { return _entries.size(); }
        /**
         * @brief Returns the entry at the given index.
         * @param i Entry index.
         */
        inline const entry& at(size_t i) const { return _entries.at(i); }
        /** @brief Returns an iterator to the first entry. */
        inline const_iterator begin() const { return _entries.begin(); }
        /** @brief Returns the past-the-end iterator. */
        inline const_iterator end() const { return _entries.end(); }

        /** @brief Defines view-data metadata for a listing. */
        friend auto metatype(udho::view::data::type<listing>){
            using namespace udho::view::data;

            return assoc("listing"),
                   cvar("label", &listing::_label),
                   fvar("subject", &listing::subject),
                   fvar("size", &listing::size),
                   iter(&listing::begin, &listing::end),
                   index(&listing::at, &listing::size);
        }
};

/**
 * @brief Collection of listings for a current URL path.
 * @ingroup DoxyG_pages
 */
class listings{
    std::vector<listing>  _collection;
    std::filesystem::path _current;
    public:
        using container_type = std::vector<listing>;
        using const_iterator = typename container_type::const_iterator;
        using value_type     = typename container_type::value_type;
        using size_type      = typename container_type::size_type;
    public:
        listings() = delete;
        listings(const listings&) = default;

        /**
         * @brief Constructs the collection for a current path.
         * @param current Current URL path.
         */
        listings(const std::string& current): _current(current) {
            assert(current[0] == '/');
        }

        /**
         * @brief Adds a listing.
         * @param l Listing to move into the collection.
         */
        inline void add(listing&& l){
            _collection.emplace_back(std::move(l));
        }

        /** @brief Returns the number of listings. */
        inline std::size_t size() const { return _collection.size(); }
        /**
         * @brief Returns the listing at the given index.
         * @param i Listing index.
         */
        inline const listing& at(size_t i) const { return _collection.at(i); }
        /** @brief Returns an iterator to the first listing. */
        inline const_iterator begin() const { return _collection.begin(); }
        /** @brief Returns the past-the-end iterator. */
        inline const_iterator end() const { return _collection.end(); }
        /** @brief Returns the current path. */
        inline std::string current() const { return _current.string(); }
        /** @brief Returns the parent of the current path. */
        inline std::string parent() const {
            std::filesystem::path p = _current.parent_path();
            if(_current.string().back() != '/'){
                return p;
            }
            return p.parent_path();
        }

        /** @brief Defines view-data metadata for a listings collection. */
        friend auto metatype(udho::view::data::type<listings>){
            using namespace udho::view::data;

            return assoc("listing"),
                   fvar("current", &listings::current),
                   fvar("parent", &listings::parent),
                   fvar("size", &listings::size),
                   iter(&listings::begin, &listings::end),
                   index(&listings::at, &listings::size);
        }
};

/**
 * @brief HTTP status data used by a listing header view.
 * @ingroup DoxyG_pages
 */
class listing_header{
    boost::beast::http::status _status;

    public:
    /**
     * @brief Constructs header data for an HTTP status.
     * @param status HTTP status.
     */
    inline explicit listing_header(boost::beast::http::status status): _status(status) {}
    /** @brief Returns the numeric status code. */
    inline int code() const { return static_cast<int>(_status); }
    /** @brief Returns the formatted status text. */
    inline std::string message() const {
        std::stringstream stream;
        stream << _status;
        return stream.str();
    }

    /** @brief Defines view-data metadata for listing header data. */
    friend auto metatype(udho::view::data::type<listing_header>){
        using namespace udho::view::data;

        return assoc("listing_header"),
               fvar("code",     &listing_header::code),
               fvar("message",  &listing_header::message);
    }
};

/**
 * @brief Collects runtime information displayed by system pages.
 * @ingroup DoxyG_pages
 */
struct status_info{
    std::string compiler;
    std::string os;
    std::string cpp;
    std::string boost;
    std::string memory;
    std::string time;

    /** @brief Collects the status information. */
    status_info() {
        compiler = compiler_info();
        os       = os_info();
        cpp      = cpp_version();
        boost    = boost_version();
        memory   = memory_available();
        time     = time_str();
    }

    /** @brief Returns compiler identification. */
    std::string compiler_info() const {
        std::ostringstream oss;
    #ifdef __clang__  // Check for Clang FIRST
        oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
    #elif defined(__GNUC__)  // Then check for GCC
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
    #elif defined(_MSC_VER)
        oss << "MSVC " << _MSC_FULL_VER;
    #else
        oss << "Unknown";
    #endif
        return oss.str();
    }

    /** @brief Returns operating-system information. */
    std::string os_info() const {
        std::ostringstream oss;

#ifdef _WIN32
        // Get Windows version information
        NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
        OSVERSIONINFOEXW osInfo;

        *(FARPROC*)&RtlGetVersion = GetProcAddress(
            GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");

        if (RtlGetVersion) {
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            if (SUCCEEDED(RtlGetVersion(&osInfo))) {
                oss << "Windows "
                    << osInfo.dwMajorVersion << "."
                    << osInfo.dwMinorVersion << "."
                    << osInfo.dwBuildNumber;
            }
        }

        // Get Windows 10+ display version
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            WCHAR displayVersion[128];
            DWORD size = sizeof(displayVersion);
            if (RegQueryValueExW(hKey, L"DisplayVersion", nullptr, nullptr,
                                 (LPBYTE)displayVersion, &size) == ERROR_SUCCESS) {
                oss << " (" << std::wstring(displayVersion) << ")";
            }
            RegCloseKey(hKey);
        }

#elif __linux__
        // Get Linux kernel version
        struct utsname buf;
        if (uname(&buf) == 0) {
            oss << "Linux " << buf.release; // << " (" << buf.version << ")";
        } else {
            oss << "Linux";
        }

        // Try to get distribution info
        std::ifstream osRelease("/etc/os-release");
        if (osRelease) {
            std::string line;
            while (std::getline(osRelease, line)) {
                if (line.find("PRETTY_NAME=") != std::string::npos) {
                    auto value = line.substr(line.find('=')+1);
                    value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
                    oss << " - " << value;
                    break;
                }
            }
        }

#elif __APPLE__
        // Get macOS version and kernel info
        struct utsname buf;
        if (uname(&buf) == 0) {
            oss << "macOS Darwin " << buf.release << " (" << buf.version << ")";
        }

        // Get macOS marketing version
        FILE* sw_vers = popen("sw_vers -productVersion", "r");
        if (sw_vers) {
            char version[128];
            if (fgets(version, sizeof(version), sw_vers) != nullptr) {
                oss << " macOS ";
                for (char* c = version; *c; c++) {
                    if (*c == '.') *c = '_';
                    if (*c == '\n') *c = '\0';
                }
                oss << version;
            }
            pclose(sw_vers);
        }

#else
        oss << "Unknown OS";
#endif

        return oss.str();
    }

    /** @brief Returns the configured C++ language version. */
    std::string cpp_version() const {
        std::ostringstream oss;
        oss << "C++" << (__cplusplus / 100 % 100);
        return oss.str();
    }

    /** @brief Returns the Boost version. */
    std::string boost_version() const {
        std::ostringstream oss;
        oss << "Boost " << BOOST_VERSION / 100000 << "."
            << BOOST_VERSION / 100 % 1000 << "."
            << BOOST_VERSION % 100;
        return oss.str();
    }

    /** @brief Returns formatted physical-memory information. */
    std::string memory_available() const {
        std::ostringstream oss;
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        oss << "RAM: " << (memInfo.ullTotalPhys >> 30) << "GB|";
#else
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        oss << "RAM: " << (memInfo.totalram * memInfo.mem_unit >> 30) << "GB";
#endif
        return oss.str();
    }

    /** @brief Returns formatted local date, time, and UTC offset. */
    std::string time_str() const{
        std::ostringstream oss;
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        oss << "UTC" << std::put_time(std::localtime(&t), "%z") << " ";
        oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    /** @brief Defines view-data metadata for status information. */
    friend auto metatype(udho::view::data::type<status_info>){
        using namespace udho::view::data;

        return assoc("status_info"),
                cvar("compiler", &status_info::compiler),
                cvar("os",       &status_info::os),
                cvar("cpp",      &status_info::cpp),
                cvar("boost",    &status_info::boost),
                cvar("memory",   &status_info::memory),
                cvar("time",     &status_info::time);
    }

};


}
}
}
}

#endif // UDHO_PAGES_DATA_H
