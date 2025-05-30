#ifndef UDHO_SESSION_STORAGE_FS_MEM_H
#define UDHO_SESSION_STORAGE_FS_MEM_H

#include <udho/session/storage/detail.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <udho/utils/filesystem.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/record_data.h>
#include <fstream>

namespace udho{
namespace session{
namespace storage{


/**
 * @brief Memory-mapped file storage for HTTP sessions
 */
class mem_fs {
    static_assert(std::is_trivially_copyable_v<udho::session::record_data::sessid_type>);
    static_assert(sizeof(detail::attr_meta)==12);

public:
    explicit mem_fs(const udho::utils::filesystem::path& root) : _root(root) {
        if (!udho::utils::filesystem::exists(_root)) {
            udho::utils::filesystem::create_directories(_root);
        }
    }

    /**
     * @brief check if the session record exists in the root directory
     * @param id session id
     * @return
     */
    bool exists(const udho::session::record_data::sessid_type& id) const {
        return udho::utils::filesystem::exists(path(id));
    }

    /**
     * @brief create the session record in the root directory
     * @param id session id
     * @return
     */
    bool create(udho::session::record_data& record) const {
        const std::size_t len = required_size(record);
        const udho::utils::filesystem::path file_path  = path(record.sessid());
        prepare_file(file_path, len);

        boost::iostreams::mapped_file_params prm;
        prm.path   = file_path.string();
        prm.length = len;
        prm.flags  = boost::iostreams::mapped_file::mapmode::readwrite;
        boost::iostreams::mapped_file mm(prm);
        if (!mm.is_open()) {
            throw std::runtime_error("mmap open failed in create()");
        }

        bool ok1 = _save(mm.data(), len, record);
        bool ok2 = _fetch(mm.const_data(), len, record);
        return ok1 && ok2;
    }

    bool fetch(udho::session::record_data& record) const {
        const auto file_path = path(record.sessid());
        if (!udho::utils::filesystem::exists(file_path)) {
            throw std::runtime_error("Session file not found");
        }
        boost::iostreams::mapped_file_source src(file_path.string());
        if (!src.is_open()) {
            throw std::runtime_error("mmap open failed in fetch()");
        }
        return _fetch(src.data(), src.size(), record);
    }

    /**
     * @brief serialize a record into disk
     * @return
     */
    bool save(const udho::session::record_data& record) const {
        const std::size_t len       = required_size(record);
        const auto        file_path = path(record.sessid());
        prepare_file(file_path, len);

        boost::iostreams::mapped_file_params prm;
        prm.path   = file_path.string();
        prm.length = len;
        prm.flags  = boost::iostreams::mapped_file::mapmode::readwrite;
        boost::iostreams::mapped_file mm(prm);
        if (!mm.is_open()) {
            throw std::runtime_error("mmap open failed in save()");
        }
        return _save(mm.data(), len, record);
    }

    udho::utils::filesystem::path path(const udho::session::record_data::sessid_type& id) const {
        return _root / session_filename(id);
    }

private:
    static std::string session_filename(const udho::session::record_data::sessid_type& id) {
        using namespace std;
        using namespace boost::uuids;
        return to_string(id) + ".udho.session";
    }

    static void prepare_file(const udho::utils::filesystem::path& file, std::size_t len) {
        std::ofstream f(file, std::ios::binary | std::ios::trunc);
        if (!f) {
            throw std::runtime_error("Failed to create session file");
        }
        f.seekp(static_cast<std::streamoff>(len - 1));
        f.put('\0');
    }

    std::size_t required_size(const udho::session::record_data& record) const {
        std::size_t size = sizeof(detail::record_preamble) + sizeof(udho::session::record_data::sessid_type);
        for (const auto& [k, v] : record) {
            size += k.size() + v.size();
        }
        size += record.size() * sizeof(detail::attr_meta);
        size += sizeof(std::uint32_t); // entry count footer
        return size;
    }


    bool _fetch(const char* src, std::size_t len, udho::session::record_data& record) const {
        if (len < detail::MIN_SIZE) {
            throw std::runtime_error("Corrupt file: too small");
        }

        const char* cursor = src;
        detail::record_preamble preamble{};
        std::memcpy(&preamble, cursor, sizeof(preamble));
        if(preamble.MAGIC != detail::SESSION_FILE_MAGIC){
            throw std::runtime_error("Magic didn't match");
        }
        if (preamble.VERSION != 1) {
            throw std::runtime_error("Unsupported file version");
        }
        record.created(preamble.created_at());
        record.updated(preamble.updated_at());
        cursor += sizeof(detail::record_preamble);

        udho::session::record_data::sessid_type sid{};
        std::memcpy(&sid, cursor, sizeof(sid));

        if (sid != record.sessid()) {
            throw std::runtime_error("Session ID mismatch");
        }
        cursor += sizeof(sid);

        const std::uint32_t entry_count = *reinterpret_cast<const std::uint32_t*>(src + len - sizeof(std::uint32_t));
        const char* metadata_base = src + len - sizeof(std::uint32_t) - entry_count * sizeof(detail::attr_meta);

        for (std::uint32_t i = 0; i < entry_count; ++i) {
            detail::attr_meta meta{};
            std::memcpy(&meta, metadata_base + i * sizeof(detail::attr_meta), sizeof(detail::attr_meta));

            if (meta.offset + meta.key_len + meta.value_len > static_cast<std::uint32_t>(metadata_base - src)) {
                throw std::runtime_error("Corrupt file: attribute out of bounds");
            }

            const char* key_ptr   = src + meta.offset;
            const char* value_ptr = key_ptr + meta.key_len;

            std::string key(key_ptr,   key_ptr   + meta.key_len);
            std::string val(value_ptr, value_ptr + meta.value_len);

            record.set(std::move(key), std::move(val), true);
        }
        return true;
    }

    bool _save(char* dst, std::size_t len, const udho::session::record_data& record) const {
        char* cursor = dst;

        detail::record_preamble preamble{};
        preamble.created_at(record.created());
        preamble.update();
        std::memcpy(cursor, &preamble, sizeof(preamble));
        cursor += sizeof(preamble);

        auto sid = record.sessid();
        std::memcpy(cursor, &sid, sizeof(sid));
        cursor += sizeof(sid);

        std::vector<detail::attr_meta> metas;
        std::uint32_t offset = static_cast<std::uint32_t>(cursor - dst);

        for (const auto& [k, v] : record) {
            std::memcpy(dst + offset, k.data(), k.size());
            std::memcpy(dst + offset + k.size(), v.data(), v.size());

            metas.push_back({ offset, static_cast<std::uint32_t>(k.size()), static_cast<std::uint32_t>(v.size()) });
            offset += static_cast<std::uint32_t>(k.size() + v.size());
        }

        const std::uint32_t entry_count = static_cast<std::uint32_t>(metas.size());
        if (entry_count) {
            std::memcpy(dst + offset, metas.data(), metas.size() * sizeof(detail::attr_meta));
            offset += metas.size() * sizeof(detail::attr_meta);
        }
        std::memcpy(dst + offset, &entry_count, sizeof(entry_count));

        if (offset + sizeof(entry_count) != len) {
            throw std::runtime_error("Size mismatch while writing mem_fs");
        }
        return true;
    }

private:
    udho::utils::filesystem::path _root;
};

}
}
}


#endif // UDHO_SESSION_STORAGE_FS_MEM_H
