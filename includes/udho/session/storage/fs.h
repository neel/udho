#ifndef UDHO_SESSION_STORAGE_FS_H
#define UDHO_SESSION_STORAGE_FS_H

#include <udho/utils/filesystem.h>
#include <fstream>
#include <udho/session/record.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/storage/detail.h>

namespace udho{
namespace session{
namespace storage{

/**
 * @brief on disk storage for HTTP session
 */
struct fs{
    static_assert(std::is_trivially_copyable_v<udho::session::record_data::sessid_type>);
    static_assert(sizeof(detail::attr_meta)==12);

    inline explicit fs(const udho::utils::filesystem::path& root): _root(root) {
        if(!udho::utils::filesystem::exists(_root)) {
            udho::utils::filesystem::create_directories(_root);
        }
    }

    /**
     * @brief check if the session record exists in the root directory
     * @param id session id
     * @return
     */
    inline bool exists(const udho::session::record_data::sessid_type& id) const {
        return udho::utils::filesystem::exists(path(id));
    }

    /**
     * @brief create the session record in the root directory
     * @param id session id
     * @return
     */
    inline bool create(udho::session::record_data& record) const {
        udho::utils::filesystem::path file_path = path(record.sessid());
        std::fstream file(file_path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!file)
            throw std::runtime_error("Failed to open file for reading");

        auto exceptbits = file.exceptions();
        file.exceptions(std::ios::failbit | std::ios::badbit);
        try{
            bool result_save = _save(file, record);
            file.seekg(0);
            bool result_load = _fetch(file, record);
            return result_save && result_load;
        } catch(const std::exception& ex) {
            file.exceptions(exceptbits);
            throw;
        }
    }

    inline bool fetch(udho::session::record_data& record) const {
        udho::utils::filesystem::path file_path = path(record.sessid());
        std::ifstream file(file_path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open file for reading");

        auto exceptbits = file.exceptions();
        file.exceptions(std::ios::failbit | std::ios::badbit);
        try{
            bool result = _fetch(file, record);
            return result;
        } catch(const std::exception& ex) {
            file.exceptions(exceptbits);
            throw;
        }
    }

    /**
     * @brief serialize a record into disk
     * @return
     */
    inline bool save(const udho::session::record_data& record) const {
        udho::utils::filesystem::path file_path = path(record.sessid());
        std::ofstream file(file_path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open session file for writing");

        auto exceptbits = file.exceptions();
        file.exceptions(std::ios::failbit | std::ios::badbit);
        try{
            bool result = _save(file, record);
            return result;
        } catch(const std::exception& ex) {
            file.exceptions(exceptbits);
            throw;
        }
    }

    private:

    /**
     * @brief _fetch the record from the storage
     * @param id session id
     * @return
     */
    template<class CharT, class Traits = std::char_traits<CharT>>
    inline bool _fetch(std::basic_istream<CharT, Traits>& file, udho::session::record_data& record) const {
        file.seekg(0, std::ios::end);
        const std::streamoff file_size = file.tellg();
        if (file_size < static_cast<std::streamoff>(detail::MIN_SIZE)) {
            throw std::runtime_error("Corrupt file: too small");
        }
        file.seekg(0, std::ios::beg);
        detail::record_preamble preamble;
        file.read(reinterpret_cast<char*>(&preamble), static_cast<std::streamsize>(sizeof(detail::record_preamble)));
        if(preamble.MAGIC != detail::SESSION_FILE_MAGIC){
            throw std::runtime_error("Magic didn't match");
        }
        if (preamble.VERSION != 1) {
            throw std::runtime_error("Unsupported file version");
        }

        udho::session::record_data::sessid_type sessid;
        file.read(reinterpret_cast<char*>(&sessid), static_cast<std::streamsize>(sizeof(udho::session::record_data::sessid_type)));

        if (sessid != record.sessid()) {
            throw std::runtime_error("Session ID mismatch");
        }

        file.seekg(0, std::ios::end);

        file.seekg(-static_cast<std::streamoff>(sizeof(std::uint32_t)), std::ios::end);
        uint32_t entry_count;
        file.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));

        if(entry_count > 0) {
            const auto metadata_start = file_size - (sizeof(uint32_t) + static_cast<std::streamoff>(entry_count * sizeof(detail::attr_meta)));
            file.seekg(metadata_start);

            std::vector<detail::attr_meta> metadata(entry_count);
            file.read(reinterpret_cast<char*>(metadata.data()), static_cast<std::streamsize>(entry_count * sizeof(detail::attr_meta)));

            for (const detail::attr_meta& meta : metadata) {
                if (meta.offset + meta.key_len + meta.value_len > metadata_start) {
                    throw std::runtime_error("Corrupt file: attribute out of bounds");
                }

                file.seekg(meta.offset); // meta.offset contains absolute offset and seekg takes absolute offset as input

                std::string key, value;
                key.resize(meta.key_len);
                value.resize(meta.value_len);

                file.read(key.data(), meta.key_len);
                file.read(value.data(), meta.value_len);

                record.set(key, std::move(value), true);
            }
        }
        return true;
    }

    /**
     * @brief serialize a record into disk
     * @return
     */
    template<class CharT, class Traits = std::char_traits<CharT> >
    inline bool _save(std::basic_ostream<CharT, Traits>& file, const udho::session::record_data& record) const {
        detail::record_preamble preamble{};
        file.write(reinterpret_cast<const char*>(&preamble), sizeof(preamble));

        udho::session::record_data::sessid_type sessid = record.sessid();
        file.write(reinterpret_cast<const char*>(&sessid), sizeof(udho::session::record_data::sessid_type));

        std::vector<detail::attr_meta> metadata;
        std::uint32_t offset = sizeof(detail::record_preamble) + sizeof(udho::session::record_data::sessid_type);

        for (const auto& [key, value] : record) {
            file.write(key.data(),   key.size());
            file.write(value.data(), value.size());

            detail::attr_meta meta{offset, static_cast<std::uint32_t>(key.size()), static_cast<std::uint32_t>(value.size())};
            metadata.emplace_back(std::move(meta)); // meta.offset contains absolute offset

            offset += key.size() + value.size();
        }

        const std::uint32_t entry_count = static_cast<std::uint32_t>(metadata.size());

        if(entry_count > 0) {
            file.write(reinterpret_cast<const char*>(metadata.data()), sizeof(detail::attr_meta) * entry_count);
        }

        file.write(reinterpret_cast<const char*>(&entry_count),    sizeof(std::uint32_t));

        file.flush();
        return true;
    }

    static std::string session_filename(const udho::session::record_data::sessid_type& id) {
        using namespace std;
        using namespace boost::uuids;
        return to_string(id) + ".udho.session";
    }

    public:
        udho::utils::filesystem::path path(const udho::session::record_data::sessid_type& id) const {
            return _root / session_filename(id);
        }

    private:
    udho::utils::filesystem::path _root;
};

}
}
}

#endif // UDHO_SESSION_STORAGE_FS_H
