#ifndef UDHO_SESSION_STORAGE_FS_H
#define UDHO_SESSION_STORAGE_FS_H

#include <udho/utils/filesystem.h>
#include <fstream>
#include <chrono>
#include <udho/session/record_data.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/storage/detail.h>
#include <udho/session/defs.h>
#include <udho/session/errors.h>
#include <udho/session/storage/features.h>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace udho{
namespace session{
namespace storage{

/**
 * @brief on disk storage for HTTP session
 */
struct fs: public udho::session::storage::features<udho::session::modes::lazy, udho::session::modes::optimistic> {
    static_assert(std::is_trivially_copyable_v<udho::session::id>);
    static_assert(sizeof(detail::attr_meta)==12);

    inline explicit fs(const udho::utils::filesystem::path& root = udho::utils::filesystem::current_path()): _root(root) {
        if(!udho::utils::filesystem::exists(_root)) {
            udho::utils::filesystem::create_directories(_root);
        }
    }

    /**
     * @brief check if the session record exists in the root directory
     * @param id session id
     * @return
     */
    inline bool exists(const udho::session::id& id) const {
        return udho::utils::filesystem::exists(path(id));
    }

    /**
     * @brief create the session record in the root directory
     * @param id session id
     * @return
     */
    inline bool create(udho::session::record_data& record) const {
        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

        udho::utils::filesystem::path file_path = path(record.sessid());
        std::fstream file(file_path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!file)
            throw udho::session::errors::io(file_path, "Failed to open file for reading");

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
        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

        udho::utils::filesystem::path file_path = path(record.sessid());
        std::ifstream file(file_path, std::ios::binary);
        if (!file)
            throw udho::session::errors::io(file_path, "Failed to open file for reading");

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
    inline bool save(const udho::session::record_data& record, bool versioning = false) const {
        // Four stages of happy path (versioning = true)
        // 1. revision_read         -> read the revision from the session file
        //                              I/O exception -> throw from std::ifstream
        // 2. revision_check        -> checks the on disk revision with the record revision and expects them to be same
        //                              version mismatch -> throw conflict
        // 3. write_temp            -> write revised record to a temporary file
        //                              I/O exception -> throw from std::ofstream
        //                              serialization exception throw from _save
        // 4. final_replace         -> replace the session file with the temp file

        // Two stages of happy path (versioning = false)
        // 1. write_temp            -> rite revised record to a temporary file
        // 2. final_replace         -> replace the session file with the temp file

        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

        udho::utils::filesystem::path file_path = path(record.sessid());
        std::uint64_t revision = 0;
        if(versioning) {
            std::ifstream file(file_path, std::ios::binary | std::ios::in);
            if (!file)
                throw udho::session::errors::io(file_path, "Failed to open session file for reading");

            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.seekg(0);
            revision = _revision(file);

            if(record.revision() != revision) {
                throw udho::session::errors::conflict{record.sessid(), revision, record.revision()};
            }
        }

        // control reaches here only under two circumstances.
        // 1. versioning is disabled
        // 2. versioning is enabled and there is no conflict

        udho::utils::filesystem::path temp_file_path = path(record.sessid(), true);
        bool result = false;
        { // RAII close
            std::ofstream tmp_file(temp_file_path, std::ios::binary | std::ios::out | std::ios::trunc);
            if (!tmp_file)
                throw udho::session::errors::io(temp_file_path, "Failed to open session temp file for writing");
            tmp_file.exceptions(std::ios::failbit | std::ios::badbit);
            result = _save(tmp_file, record);

            if(!result){
                throw udho::session::errors::io{temp_file_path, "Failed to save sessiion to a temporary file"};
            }
        }

        udho::utils::filesystem::remove(file_path);
        udho::utils::filesystem::rename(temp_file_path, file_path);
        return result;
    }

    inline bool remove(udho::session::record_data& record) {
        udho::utils::filesystem::path lock_file_path = lock_path(record.sessid());
        boost::interprocess::file_lock lk(lock_file_path.c_str());
        boost::interprocess::scoped_lock< boost::interprocess::file_lock > guard(lk);

        udho::utils::filesystem::path file_path = path(record.sessid());
        bool result = udho::utils::filesystem::remove(file_path);
        return result;
    }

    private:

    /**
     * @brief _preamble reads proamble and sets it to the out parameter, throws in case the file is too small, magic is invalid or version is unsupported
     * @param file
     * @param preamble
     * @return size of the file read
     */
    template<class CharT, class Traits = std::char_traits<CharT>>
    inline std::streamoff _preamble(std::basic_istream<CharT, Traits>& file, detail::record_preamble& preamble) const {
        file.seekg(0, std::ios::end);
        const std::streamoff file_size = file.tellg();
        if (file_size < static_cast<std::streamoff>(detail::MIN_SIZE)) {
            throw udho::session::errors::corruption::too_small();
        }
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&preamble), static_cast<std::streamsize>(sizeof(detail::record_preamble)));
        if(preamble.MAGIC != detail::SESSION_FILE_MAGIC){
            throw udho::session::errors::corruption::magic_invalid();
        }
        if (preamble.VERSION != 1) {
            throw udho::session::errors::corruption::unsupported_version();
        }
        return file_size;
    }


    template<class CharT, class Traits = std::char_traits<CharT>>
    inline std::uint64_t _revision(std::basic_istream<CharT, Traits>& file) const {
        detail::record_preamble preamble;
        _preamble(file, preamble);
        return preamble.get_revision();
    }

    /**
     * @brief _fetch the record from the storage
     * @param id session id
     * @return
     */
    template<class CharT, class Traits = std::char_traits<CharT>>
    inline bool _fetch(std::basic_istream<CharT, Traits>& file, udho::session::record_data& record) const {
        detail::record_preamble preamble;
        std::streamoff file_size = _preamble(file, preamble);
        record.created(preamble.created_at());
        record.updated(preamble.updated_at());
        record.revision(preamble.get_revision());

        udho::session::id sessid;
        file.read(reinterpret_cast<char*>(&sessid), static_cast<std::streamsize>(sizeof(udho::session::id)));

        if (sessid != record.sessid()) {
            throw udho::session::errors::corruption::sessid_mismatch();
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
                    throw udho::session::errors::corruption::invalid_offset();
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
        detail::record_preamble::time_type current_time = std::chrono::system_clock::now();

        detail::record_preamble preamble{};
        preamble.created_at(record.created());
        preamble.update(current_time);
        preamble.set_revision(record.revision() +1);
        file.write(reinterpret_cast<const char*>(&preamble), sizeof(preamble));

        udho::session::id sessid = record.sessid();
        file.write(reinterpret_cast<const char*>(&sessid), sizeof(udho::session::id));

        std::vector<detail::attr_meta> metadata;
        std::uint32_t offset = sizeof(detail::record_preamble) + sizeof(udho::session::id);

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

        bool result = file.good();

        if(result){
            auto& mutable_record = const_cast<udho::session::record_data&>(record);
            mutable_record.sync(preamble.get_revision(), current_time);
        }

        return result;
    }


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

    public:
        udho::utils::filesystem::path path(const udho::session::id& id, bool temporary = false) const {
            return _root / session_filename(id, temporary);
        }

    private:
    udho::utils::filesystem::path _root;
};

}
}
}

#endif // UDHO_SESSION_STORAGE_FS_H
