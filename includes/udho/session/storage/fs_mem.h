#ifndef UDHO_SESSION_STORAGE_FS_MEM_H
#define UDHO_SESSION_STORAGE_FS_MEM_H

#include <udho/session/storage/detail.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <udho/utils/filesystem.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/record_data.h>
#include <udho/session/errors.h>
#include <fstream>
#include <udho/session/storage/features.h>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace udho{
namespace session{
namespace storage{

/**
 * @brief Memory-mapped file storage for HTTP sessions
 */
struct mem_fs: public udho::session::storage::features<udho::session::modes::lazy, udho::session::modes::optimistic> {
    static_assert(std::is_trivially_copyable_v<udho::session::id>);
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
    bool exists(const udho::session::id& id) const {
        return udho::utils::filesystem::exists(path(id));
    }

    /**
     * @brief create the session record in the root directory
     * @param id session id
     * @return
     */
    bool create(udho::session::record_data& record) const {
        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

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
        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

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
    bool save(const udho::session::record_data& record, bool versioning = false) const {
        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

        const std::size_t len = required_size(record);
        const udho::utils::filesystem::path file_path = path(record.sessid());

        std::uint64_t revision = 0;
        if(versioning) {
            boost::iostreams::mapped_file_source src(file_path.string());
            if (!src.is_open()) {
                throw std::runtime_error("mmap open failed in fetch()");
            }

            revision = _revision(src.data(), src.size());

            if(record.revision() != revision) {
                throw udho::session::errors::conflict{record.sessid(), revision, record.revision()};
            }
        }

        const udho::utils::filesystem::path temp_file_path = path(record.sessid(), true);

        prepare_file(temp_file_path, len);
        boost::iostreams::mapped_file_params prm;
        prm.path    = temp_file_path.string();
        prm.length  = len;
        prm.flags   = boost::iostreams::mapped_file::mapmode::readwrite;
        bool result = false;
        try{
            boost::iostreams::mapped_file mm(prm);
            if (!mm.is_open()) {
                throw std::runtime_error("mmap open failed in save()");
            }
            result = _save(mm.data(), len, record);
        } catch(const std::exception& ex){
            udho::utils::filesystem::remove(temp_file_path);
            throw;
        }

        if(!result){
            udho::utils::filesystem::remove(temp_file_path);
            throw std::runtime_error{"Failed to save session to a temporary file"};
        }

        udho::utils::filesystem::remove(file_path);
        udho::utils::filesystem::rename(temp_file_path, file_path);
        return result;
    }

    udho::utils::filesystem::path path(const udho::session::id& id, bool temporary = false) const {
        return _root / session_filename(id, temporary);
    }


private:
    static std::string session_filename(const udho::session::id& id, bool temporary = false) {
        std::string extension = ".udho.session";
        if(temporary) {
            extension += ".tmp";
        }
        return udho::session::to_string(id) + extension;
    }

    udho::utils::filesystem::path lock_path(const udho::session::id& id) const {
        udho::utils::filesystem::path path = _root / (session_filename(id, false) + ".lck");
        if(!udho::utils::filesystem::exists(path)) {
            std::ofstream touch(path);
        }
        return path;
    }

    static void prepare_file(const udho::utils::filesystem::path& file, std::size_t len) {
        std::ofstream f(file, std::ios::binary);
        if (!f) {
            throw std::runtime_error("Failed to create session file");
        }
        f.seekp(static_cast<std::streamoff>(len - 1));
        f.put('\0');
   }

    std::size_t required_size(const udho::session::record_data& record) const {
        std::size_t size = sizeof(detail::record_preamble) + sizeof(udho::session::id);
        for (const auto& [k, v] : record) {
            size += k.size() + v.size();
        }
        size += record.size() * sizeof(detail::attr_meta);
        size += sizeof(std::uint32_t); // entry count footer
        return size;
    }

    const char* _preamble(const char* src, std::size_t len, detail::record_preamble& preamble) const {
        if (len < detail::MIN_SIZE) {
            throw std::runtime_error("Corrupt file: too small");
        }

        const char* cursor = src;
        std::memcpy(&preamble, cursor, sizeof(preamble));
        if(preamble.MAGIC != detail::SESSION_FILE_MAGIC){
            throw std::runtime_error("Magic didn't match");
        }
        if (preamble.VERSION != 1) {
            throw std::runtime_error("Unsupported file version");
        }
        cursor += sizeof(detail::record_preamble);
        return cursor;
    }

    inline std::uint64_t _revision(const char* src, std::size_t len) const {
        detail::record_preamble preamble;
        _preamble(src, len, preamble);
        return preamble.revision;
    }

    bool _fetch(const char* src, std::size_t len, udho::session::record_data& record) const {
        detail::record_preamble preamble{};
        const char* cursor = _preamble(src, len, preamble);
        record.created(preamble.created_at());
        record.updated(preamble.updated_at());
        record.revision(preamble.revision);
        udho::session::id sid{};
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

            std::size_t limit = static_cast<std::size_t>(metadata_base - src);
            if (static_cast<std::size_t>(meta.offset) + meta.key_len + meta.value_len > limit) {
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
        detail::record_preamble::time_type current_time = std::chrono::system_clock::now();

        detail::record_preamble preamble{};
        preamble.created_at(record.created());
        preamble.update(current_time);
        preamble.revision = record.revision() +1;
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
            throw std::runtime_error("Size mismatch while writing");
        }

        auto& mutable_record = const_cast<udho::session::record_data&>(record);
        mutable_record.revision(preamble.revision);
        mutable_record.updated(current_time);

        return true;
    }

private:
    udho::utils::filesystem::path _root;
};

}
}
}


#endif // UDHO_SESSION_STORAGE_FS_MEM_H
