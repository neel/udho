#ifndef UDHO_SESSION_STORAGE_REDIS_H
#define UDHO_SESSION_STORAGE_REDIS_H

#ifdef WITH_HIREDIS

#include <udho/session/record_data.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/storage/detail.h>
#include <udho/session/defs.h>
#include <udho/session/errors.h>
#include <udho/session/record_data.h>
#include <udho/session/storage/features.h>
#include <hiredis/hiredis.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <variant>
#include <udho/utils/format.h>
#include <udho/utils/string_view.h>

#ifndef REDIS_REPLY_VERB
#define REDIS_REPLY_VERB 0x1000
#endif
#ifndef REDIS_REPLY_BIGNUM
#define REDIS_REPLY_BIGNUM 0x1001
#endif
#ifndef REDIS_REPLY_BOOL
#define REDIS_REPLY_BOOL REDIS_REPLY_INTEGER
#endif

namespace udho{
namespace session{
namespace storage{

namespace detail{

/**
 * @brief The internal redis_commander class
 * expects that the caller knows the expected result type
 */
struct redis_commander{
    enum class resp_type {
        nil, status, string, integer, double_, bool_,
        array, set, push, map, attr, bignum, verb
    };

    struct command_response {
        command_response()  = default;
        ~command_response() = default;
        command_response(command_response&&)            noexcept = default;
        command_response& operator=(command_response&&) noexcept = default;
        command_response(const command_response&)  = delete;
        command_response& operator=(const command_response&) = delete;

        resp_type type = resp_type::nil;

        using collection_type   = std::vector<command_response>;
        using map_type          = std::vector<std::pair<command_response,command_response>>;
        using payload_type      = std::variant<
                                    std::monostate,           // nil
                                    std::string,              // status, string, bignum, verb
                                    std::int64_t,             // integer
                                    double,                   // double
                                    bool,                     // bool
                                    collection_type,          // array, set, push
                                    map_type                  // map, attr
                                >;
        payload_type data;

        template <resp_type respt>
        bool is() const { return type == respt; }

        bool   is_nil()     const { return type == resp_type::nil;     }
        bool   is_status()  const { return type == resp_type::status;  }
        bool   is_string()  const { return type == resp_type::string;  }
        bool   is_int()     const { return type == resp_type::integer; }
        bool   is_double()  const { return type == resp_type::double_; }
        bool   is_bool()    const { return type == resp_type::bool_;   }
        bool   is_array()   const { return type == resp_type::array;   }
        bool   is_set()     const { return type == resp_type::set;     }
        bool   is_push()    const { return type == resp_type::push;    }
        bool   is_map()     const { return type == resp_type::map;     }
        bool   is_attr()    const { return type == resp_type::attr;    }


        const std::string&     as_str()    const { return std::get<std::string>(data); }
        std::int64_t           as_int()    const { return std::get<std::int64_t>(data);}
        double                 as_double() const { return std::get<double>(data);      }
        bool                   as_bool()   const { return std::get<bool>(data);        }
        const collection_type& as_array()  const { return std::get<collection_type>(data);     }
        const map_type&        as_map()    const { return std::get<map_type>(data);       }
    };

    inline explicit redis_commander() { }

    template<typename... Args>
    void queue(redisContext* redis, const std::string& cmd, Args&&... args) {
        std::vector<std::string> arg_storage;
        std::vector<const char*> argv;
        std::vector<size_t> argvlen;

        argv.reserve(1 + sizeof...(Args));
        argvlen.reserve(1 + sizeof...(Args));
        arg_storage.reserve(1 + sizeof...(Args));

        auto store_arg = [&](auto&& arg) {
            arg_storage.emplace_back(std::forward<decltype(arg)>(arg));
            argvlen.push_back(arg_storage.back().size());
            argv.push_back(arg_storage.back().c_str());
        };

        argv.push_back(cmd.c_str());
        argvlen.push_back(cmd.size());

        (store_arg(std::forward<Args>(args)), ...);

        int rc = redisAppendCommandArgv(redis, argv.size(), argv.data(), argvlen.data());
        if (rc != REDIS_OK) {
            throw std::runtime_error{"redisAppendCommandArgv failed"};
        }
    }

    command_response convert(redisReply* reply) {
        command_response response;
        switch (reply->type) {
            case REDIS_REPLY_STATUS:
                response.type  = resp_type::status;
                response.data  = std::string(reply->str, reply->len);
                break;

            case REDIS_REPLY_STRING:
                response.type  = resp_type::string;
                response.data  = std::string(reply->str, reply->len);
                break;

            case REDIS_REPLY_BIGNUM:
                response.type  = resp_type::bignum;
                response.data  = std::string(reply->str, reply->len);
                break;

            case REDIS_REPLY_VERB:
                response.type  = resp_type::verb;
                response.data  = std::string(reply->str, reply->len);
                break;

            case REDIS_REPLY_INTEGER:
                response.type  = resp_type::integer;
                response.data  = static_cast<std::int64_t>(reply->integer);
                break;

#if defined(REDIS_REPLY_BOOL) && (REDIS_REPLY_BOOL != REDIS_REPLY_INTEGER)

            case REDIS_REPLY_BOOL:
                response.type  = resp_type::bool_;
                response.data  = static_cast<bool>(reply->integer);
                break;

#endif

            case REDIS_REPLY_DOUBLE:
                response.type  = resp_type::double_;
#if HIREDIS_MAJOR > 1 || (HIREDIS_MAJOR == 1 && HIREDIS_MINOR >= 1)
                response.data = reply->dval;
#else
                response.data = std::strtod(reply->str, nullptr);
#endif
                break;

            case REDIS_REPLY_NIL:
                response.type  = resp_type::nil;
                break;

            case REDIS_REPLY_ARRAY:
            case REDIS_REPLY_SET:
            case REDIS_REPLY_PUSH: {
                command_response::collection_type vec;
                vec.reserve(reply->elements);
                for (size_t i = 0; i < reply->elements; ++i) {
                    vec.emplace_back(convert(reply->element[i]));
                }
                response.type = (reply->type == REDIS_REPLY_ARRAY) ? resp_type::array
                                                                   : (reply->type == REDIS_REPLY_SET) ? resp_type::set
                                                                                                      : resp_type::push;
                response.data = std::move(vec);
                break;
            }

            case REDIS_REPLY_MAP:
            case REDIS_REPLY_ATTR: {
                command_response::map_type mp;
                for (size_t i = 0; i + 1 < reply->elements; i += 2) {
                    mp.emplace_back(convert(reply->element[i]), convert(reply->element[i + 1]));
                }
                response.type = (reply->type == REDIS_REPLY_MAP) ? resp_type::map
                                                                 : resp_type::attr;
                response.data = std::move(mp);
                break;
            }

            default:
                throw std::runtime_error{ "Unsupported redis reply type: " + std::to_string(reply->type)};

        }
        return response;
    }

    command_response execute(redisContext* redis) {
        redisReply* reply = nullptr;
        if(redisGetReply(redis,(void**)&reply)  != REDIS_OK || reply == nullptr) {
            throw std::runtime_error{"redisGetReply failed: " + std::string(redis->errstr)};
        }

        std::unique_ptr<redisReply, decltype(&freeReplyObject)> raii_reply{reply, &freeReplyObject};

        if (reply->type == REDIS_REPLY_ERROR) {
            throw std::runtime_error{"Redis-Error: " + std::string(reply->str, reply->len)};
        }

        return convert(reply);
    }

};

}

struct redis: public udho::session::storage::features<udho::session::modes::lazy, udho::session::modes::optimistic, udho::session::modes::immediate> {

    inline explicit redis(const std::string& host = "localhost", std::uint32_t port = 6379) {
        _redis = redisConnect(host.c_str(), port);
        if (!_redis || _redis->err) {
            std::string err = _redis ? _redis->errstr : "redisConnect returned null";
            throw std::runtime_error{udho::utils::format("Failed to connect to redis {}:{} with error {}", host, port, err)};
        }
    }

    redis(const redis&) = delete;
    inline redis(redis&& other): _redis(nullptr), _commander(std::move(other._commander)) { std::swap(_redis, other._redis); }

    ~redis() { if (_redis) redisFree(_redis); }

    inline bool exists(const udho::session::id& sid) const {
        const std::string key = "sess:" + udho::session::to_string(sid);
        detail::redis_commander::command_response r = _command("EXISTS", key);
        if(r.is_int()){
            return r.as_int() != 0;
        } else if(r.is_bool()) {
            return r.as_bool();
        } else {
            throw std::runtime_error("EXISTS returned unexpected type");
        }
        return false;
    }

    inline bool create(udho::session::record_data& record) {
        try{
            bool result_save = _save(record);
            bool result_load = fetch(record);
            return result_save && result_load;
        } catch(const std::exception& ex) {
            throw;
        }
    }

    inline bool fetch(udho::session::record_data& record) {
        auto handle_pair = [&](std::string_view field, std::string_view value){
            if (field == "created") {
                auto t = std::stoll(std::string(value));
                record.created(udho::session::time_point{std::chrono::nanoseconds{t} } );
            }
            else if (field == "updated"){
                auto t = std::stoll(std::string(value));
                record.updated(udho::session::time_point{std::chrono::nanoseconds{t} } );
            }
            else if (field == "revision"){
                auto rev = static_cast<std::uint64_t>(std::stoull(std::string(value)));
                record.revision(rev);
            }
            else {
                record.set(std::string(field), std::string(value), /*initial=*/true);
            }
        };

        const std::string key = std::string("sess:")  + udho::session::to_string(record.sessid());
        detail::redis_commander::command_response response = _command("HGETALL", key);
        if(response.is_map()) {
            const detail::redis_commander::command_response::map_type& map = response.as_map();
            for (auto const& kv : map) {
                auto f = kv.first .as_str();
                auto v = kv.second.as_str();
                handle_pair(f, v);
            }
        } else if(response.is_array()) {
            const detail::redis_commander::command_response::collection_type& collection = response.as_array();
            if(collection.size() == 0) {
                throw std::runtime_error("HGETALL returned unexpected reply type: " + std::to_string(static_cast<int>(response.type)) );
            } else {
                for (size_t i = 0; i + 1 < collection.size(); i += 2) {
                    auto const& field_r = collection[i];
                    auto const& value_r = collection[i+1];
                    handle_pair(field_r.as_str(), value_r.as_str());
                }
            }
        }
        return true;
    }


    inline bool save(const udho::session::record_data& record, bool versioning = false) const {
        auto now = std::chrono::system_clock::now();
        const std::string key = std::string("sess:")  + udho::session::to_string(record.sessid());

        std::uint64_t revision = 0;
        if (versioning) {
            _watch(key);

            auto hget_revision_response = _command("HGET", key, "revision");
            if (hget_revision_response.is_nil()) {
                revision = 0;
            } else if (hget_revision_response.is_int()) {
                revision = static_cast<std::uint64_t>(hget_revision_response.as_int());
            } else if (hget_revision_response.is_string()) {
                revision = std::stoull(hget_revision_response.as_str());
            } else {
                _command("UNWATCH");
                throw std::runtime_error("Unexpected type from HGET revision");
            }

            if (record.revision() != revision) {
                _command("UNWATCH");
                throw udho::session::errors::conflict{record.sessid(), revision, record.revision()};
            }
        }

        bool result = _save(record);
        if(!result) {
            if(versioning)
                throw udho::session::errors::conflict{record.sessid(), revision, record.revision()};
            else
                throw std::runtime_error{"EXEC returned nil"};
        }
        return result;
    }

    inline bool remove(udho::session::record_data& record) {
        const std::string key = std::string("sess:")  + udho::session::to_string(record.sessid());
        detail::redis_commander::command_response r = _command("DEL", key);
        if(r.is_int()){
            return r.as_int() == 1;
        } else if(r.is_bool()) {
            return r.as_bool();
        } else {
            throw std::runtime_error("DEL returned unexpected type");
        }
        return false;
    }

    template<typename... Args>
    detail::redis_commander::command_response _command(const std::string& cmd, Args&&... args) const {
        _queue(cmd, std::forward<Args>(args)...);
        return _execute();
    }

private:
    template<typename... Args>
    const redis& _queue(const std::string& cmd, Args&&... args) const {
        const_cast<detail::redis_commander&>(_commander).queue(_redis, cmd, std::forward<Args>(args)...);
        return *this;
    }

    detail::redis_commander::command_response _execute() const {
        return const_cast<detail::redis_commander&>(_commander).execute(_redis);
    }

    template<typename... Args>
    inline void _watch(Args&&... args) const {
        detail::redis_commander::command_response watch_response = _command("WATCH", std::forward<Args>(args)...);

        if(!watch_response.is_status()){
            _command("UNWATCH");
            throw std::runtime_error{udho::utils::format("WATCH didn't return a status")};
        } else if(watch_response.as_str() != "OK") {
            _command("UNWATCH");
            throw std::runtime_error{udho::utils::format("WATCH returned {} expecting OK", watch_response.as_str())};
        }
    }

    inline bool _save(const udho::session::record_data& record) const {
        const std::string key = std::string("sess:")  + udho::session::to_string(record.sessid());
        detail::record_preamble::time_type current_time = std::chrono::system_clock::now();

        detail::redis_commander::command_response multi_response = _command("MULTI");
        if(!multi_response.is_status()){
            throw std::runtime_error{udho::utils::format("MULTI didn't retun a status")};
        } else if(multi_response.as_str() != "OK") {
            throw std::runtime_error{udho::utils::format("MULTI returned {} expecting OK", multi_response.as_str())};
        }
        std::size_t nsends = 0;
        {
            auto created_ns = record.created().time_since_epoch().count();
            _queue("HSET", key, "created", std::to_string(created_ns));
            nsends++;
            auto updated_ns = current_time.time_since_epoch().count();
            _queue("HSET", key, "updated", std::to_string(updated_ns));
            nsends++;
            std::uint64_t new_rev = record.revision() + 1;
            _queue("HSET", key, "revision", std::to_string(new_rev));
            nsends++;
        }
        for (const auto& k : record.removed_fields()) {
            _queue("HDEL", key, k);
            nsends++;
        }
        for (const auto& [k, v] : record) {
            if(record.is_updated(k)) {
                _queue("HSET", key, k, v);
                nsends++;
            }
        }
        _queue("EXEC");

        // fetch N status lines for the HSETs because MULTI was executed with command
        for(auto i = 0; i < nsends; ++i){
            _execute();
        }
        auto exec_resp = _execute();  // this fetches the EXEC reply
        bool result = !exec_resp.is_nil();
        if(result){
            auto& mutable_rec = const_cast<udho::session::record_data&>(record);
            mutable_rec.sync(record.revision() + 1, current_time);
        }

        return result;
    }

private:
    redisContext* _redis;
    detail::redis_commander _commander;
};

}
}
}

#endif // WITH_HIREDIS

#endif // UDHO_SESSION_STORAGE_REDIS_H
