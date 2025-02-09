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

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace udho{
namespace pages{
namespace system{
namespace data{

class file_entry{
    std::filesystem::path           _path;
    std::filesystem::file_status    _status;
    std::filesystem::file_type      _type;
    std::filesystem::perms          _permissions;
    bool                            _is_directory;
    std::uintmax_t                  _size;
    std::filesystem::file_time_type _modification_time;
    std::filesystem::path           _root;

    public:
        inline file_entry(const std::filesystem::directory_entry& entry, const std::filesystem::path& root)
            : _path (entry.path()), _status(entry.status()), _type(_status.type()), _permissions(_status.permissions())
            , _is_directory(entry.is_directory()) , _size(_is_directory ? 0 : entry.file_size()), _modification_time(entry.last_write_time()), _root(root)
            {}
        inline bool is_directory() const { return _is_directory; }
        inline std::string filename() const { return _path.filename(); }
        inline std::string extension() const { return _path.extension(); }

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
            oss << std::fixed << std::setprecision(2) << size << ' ' << units[unit];
            return oss.str();
        }

        inline std::string type() const {
            switch (_type) {
                case std::filesystem::file_type::regular:    return "File";
                case std::filesystem::file_type::directory:  return "Directory";
                case std::filesystem::file_type::symlink:    return "Symlink";
                case std::filesystem::file_type::block:      return "Block Device";
                case std::filesystem::file_type::character:  return "Character Device";
                case std::filesystem::file_type::fifo:       return "FIFO";
                case std::filesystem::file_type::socket:     return "Socket";
                default:                                     return "Unknown";
            }
        }

        inline std::string permissions() const {
            auto perm_to_char = [](std::filesystem::perms p, char rwx) {
                return (p & std::filesystem::perms::mask) == std::filesystem::perms::none
                    ? '-' : rwx;
            };

            std::string result(9, '-');
            result[0] = perm_to_char(_permissions & std::filesystem::perms::owner_read,  'r');
            result[1] = perm_to_char(_permissions & std::filesystem::perms::owner_write, 'w');
            result[2] = perm_to_char(_permissions & std::filesystem::perms::owner_exec,  'x');
            result[3] = perm_to_char(_permissions & std::filesystem::perms::group_read,  'r');
            result[4] = perm_to_char(_permissions & std::filesystem::perms::group_write, 'w');
            result[5] = perm_to_char(_permissions & std::filesystem::perms::group_exec,  'x');
            result[6] = perm_to_char(_permissions & std::filesystem::perms::others_read, 'r');
            result[7] = perm_to_char(_permissions & std::filesystem::perms::others_write,'w');
            result[8] = perm_to_char(_permissions & std::filesystem::perms::others_exec, 'x');
            return result;
        }

        inline std::string modification_time() const {
            auto sctp    = std::chrono::time_point_cast<std::chrono::system_clock::duration>(_modification_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto ttime   = std::chrono::system_clock::to_time_t(sctp);
            std::tm* gmt = std::gmtime(&ttime);

            std::stringstream buffer;
            buffer << std::put_time(gmt, "%A, %d %B %Y %H:%M");

            return buffer.str();
        }

        inline std::string url() const {
            std::filesystem::path relative = std::filesystem::relative(_path, _root);

            std::string url_path = relative.string();
            std::replace(url_path.begin(), url_path.end(), '\\', '/');

            if (url_path.empty()) {
                return "/";
            }

            return (url_path.front() == '/') ? url_path : "/" + url_path;
        }

        friend auto metatype(udho::view::data::type<file_entry>){
            using namespace udho::view::data;

            return assoc("file_entry"),
                fvar("url",         &file_entry::url),
                fvar("name",        &file_entry::filename),
                fvar("extension",   &file_entry::extension),
                fvar("is_dir",      &file_entry::is_directory),
                fvar("type",        &file_entry::type),
                fvar("permissions", &file_entry::permissions),
                fvar("file_size",   &file_entry::size),
                fvar("modified_at", &file_entry::modification_time);
        }

};

class directory_listing{
    std::filesystem::path   _path;
    std::vector<file_entry> _entries;
    std::filesystem::path   _root;

    public:
        using container_type = std::vector<file_entry>;
        using const_iterator = typename container_type::const_iterator;
        using value_type     = typename container_type::value_type;
        using size_type      = typename container_type::size_type;
    public:
        inline directory_listing(const std::filesystem::path& path, const std::filesystem::path& root): _path(path), _root(root) {
            std::filesystem::directory_iterator dit{_path};
            for(const std::filesystem::directory_entry& entry: dit){
                _entries.emplace_back(file_entry{entry, _root});
            }
        }

        inline std::string path() const { return _path; }
        inline std::size_t size() const { return _entries.size(); }
        inline const file_entry& at(size_t i) const { return _entries.at(i); }
        inline const_iterator begin() const { return _entries.begin(); }
        inline const_iterator end() const { return _entries.end(); }

        friend auto metatype(udho::view::data::type<directory_listing>){
            using namespace udho::view::data;

            return assoc("directory_listing"),
                fvar("path", &directory_listing::path),
                fvar("size", &directory_listing::size),
                iter(&directory_listing::begin, &directory_listing::end),
                index(&directory_listing::at, &directory_listing::size);
        }
};

class listing_header{
    std::filesystem::path _path;
    std::filesystem::path _root;
    public:
        inline listing_header(const std::filesystem::path& path, const std::filesystem::path& root): _path(path), _root(root) {}
        inline std::string url() const {
            std::filesystem::path relative = std::filesystem::relative(_path, _root);

            std::string url_path = relative.string();
            std::replace(url_path.begin(), url_path.end(), '\\', '/');

            if (url_path.empty()) {
                return "/";
            }

            return (url_path.front() == '/') ? url_path : "/" + url_path;
        }
        friend auto metatype(udho::view::data::type<listing_header>){
            using namespace udho::view::data;

            return assoc("listing_header"),
                   fvar("base", &listing_header::url);
        }
};

struct status_info{
    std::string compiler;
    std::string os;
    std::string cpp;
    std::string boost;
    std::string memory;
    std::string time;

    status_info() {
        compiler = compiler_info();
        os       = os_info();
        cpp      = cpp_version();
        boost    = boost_version();
        memory   = memory_available();
        time     = time_str();
    }

    std::string compiler_info() const {
        std::ostringstream oss;
#ifdef __GNUC__
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(__clang__)
        oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(_MSC_VER)
        oss << "MSVC " << _MSC_FULL_VER;
#else
        oss << "Unknown";
#endif
        return oss.str();
    }

    std::string os_info() const {
        std::ostringstream oss;
#ifdef _WIN32
        OSVERSIONINFOEX osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx((OSVERSIONINFO*)&osvi);
        oss << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
#elif __linux__
        oss << "Linux";
#elif __APPLE__
        oss << "macOS";
#else
        oss << "Unknown OS";
#endif
        return oss.str();
    }

    std::string cpp_version() const {
        std::ostringstream oss;
        oss << "C++" << (__cplusplus / 100 % 100);
        return oss.str();
    }

    std::string boost_version() const {
        std::ostringstream oss;
        oss << "Boost " << BOOST_VERSION / 100000 << "."
            << BOOST_VERSION / 100 % 1000 << "."
            << BOOST_VERSION % 100;
        return oss.str();
    }

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

    std::string time_str() const{
        std::ostringstream oss;
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        oss << "Timezone: UTC" << std::put_time(std::localtime(&t), "%z") << "|";
        oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

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
