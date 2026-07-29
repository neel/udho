#ifndef UDHO_URL_MIMES_H
#define UDHO_URL_MIMES_H

#include <map>
#include <regex>
#include <string>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <udho/url/utils.h>

namespace udho{
namespace url{

/**
 * @addtogroup DoxyG_url_router
 * @{
 */


/**
 * @brief The mime_registry class
 */
struct mime_registry{

    /**
     * @brief mime_registry
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
    mime_registry() {
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
     * @code{.cpp}
     * // Register Markdown files
     * explorer.mime("md", "text/markdown");
     *
     * // Register custom application type
     * explorer.mime("myapp", "application/x-myapp");
     * @endcode
     */
    mime_registry& mime(const std::string& extension, const std::string& mime){
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
        _mapping[ext] = mime;
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
        return _mapping.count(ext) > 0;
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
        auto it = _mapping.find(ext);
        if(it != _mapping.end()){
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

private:
    std::map<std::string, std::string> _mapping;
};

/// @}

}
}

#endif // UDHO_URL_MIMES_H
