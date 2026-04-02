#ifndef UDHO_LOGGING_SERIALIZATION_H
#define UDHO_LOGGING_SERIALIZATION_H

#include <string>
#include <nlohmann/json.hpp>
#include <udho/utils/traits.h>
#include <boost/asio/ip/address.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/version.hpp>

namespace udho {
namespace logging {

namespace detail {
struct binary_serializer {
    using storage_type = std::vector<std::uint8_t>;

    explicit binary_serializer(storage_type& storage) : _storage(storage) {}

    template <typename ParamT>
    void operator()(const ParamT& d) {
        const auto& v = d.value();
        using ValueType = typename ParamT::value_type;

        if constexpr (udho::utils::traits::is_optional<ValueType>::value) {
            if (v.has_value()) {
                _storage.push_back(0x01);
                write_value(v.value());
            } else {
                _storage.push_back(0x00);
            }
        } else {
            // mandatory field: just write the value (no presence flag)
            write_value(v);
        }
    }

private:
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    void write_value(const T& val) {
        auto begin = reinterpret_cast<const std::uint8_t*>(&val);
        auto end   = begin + sizeof(T);
        _storage.insert(_storage.end(), begin, end);
    }

    template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    void write_value(const T& val) {
        using underlying_type = std::underlying_type_t<T>;

        underlying_type underlying_value = static_cast<underlying_type>(val);
        write_value(underlying_value);
    }

    void write_value(const std::string& val) {
        std::uint32_t len = static_cast<std::uint32_t>(val.size());
        auto len_begin = reinterpret_cast<const std::uint8_t*>(&len);
        auto len_end   = len_begin + sizeof(len);
        _storage.insert(_storage.end(), len_begin, len_end);
        _storage.insert(_storage.end(), val.begin(), val.end());
    }

    template <typename Clock, typename Duration>
    void write_value(const std::chrono::time_point<Clock, Duration>& val) {
        write_value(val.time_since_epoch());
    }

    template <typename T, typename Ratio>
    void write_value(const std::chrono::duration<T, Ratio>& val) {
        write_value(val.count());
    }

    void write_value(const boost::uuids::uuid& uuid) {
#if BOOST_VERSION >= 108600
        auto const* begin = uuid.data();
#else
        auto const* begin = uuid.data;
#endif
        auto end   = begin + uuid.size();
        _storage.insert(_storage.end(), begin, end);
    }

    void write_value(const boost::asio::ip::address& addr) {
        if (addr.is_v4()) {
            _storage.push_back(0x04);
            auto v4 = addr.to_v4();
            auto bytes = v4.to_bytes();
            _storage.insert(_storage.end(), bytes.begin(), bytes.end());
        } else {
            _storage.push_back(0x06);
            auto v6 = addr.to_v6();
            auto bytes = v6.to_bytes();
            _storage.insert(_storage.end(), bytes.begin(), bytes.end());
        }
    }

    storage_type& _storage;
};

struct binary_deserializer {
    using pointer_type = const std::uint8_t*;

    binary_deserializer(pointer_type& data, std::size_t length): _begin(data), _end(data + length) {}

    template <typename ParamT>
    void operator()(ParamT& d) {
        using ValueType = typename ParamT::value_type;

        if constexpr (udho::utils::traits::is_optional<ValueType>::value) {
            std::size_t distance = std::distance(_begin, _end);
            if(distance < 1 ) {
                throw std::invalid_argument{"corrupt data"};
            }
            std::uint8_t present = *_begin++;
            if (present == 0x01) {
                typename ValueType::value_type val;
                if (!read_value(val)) {
                    throw std::invalid_argument{"corrupt data"};
                }
                d = val;
            } else if (present == 0x00) {
                d = std::nullopt;
            } else {
                throw std::invalid_argument{"corrupt data"};
            }
        } else {
            ValueType val;
            if (!read_value(val)) {
                throw std::invalid_argument{"corrupt data"};
            }
            d = val;
        }
    }

private:
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    bool read_value(T& val) {
        std::size_t distance = std::distance(_begin, _end);
        if (distance < sizeof(T)) return false;
        std::memcpy(&val, _begin, sizeof(T));
        _begin += sizeof(T);
        return true;
    }

    template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    bool read_value(T& val) {
        using underlying_type = std::underlying_type_t<T>;

        underlying_type underlying_value = static_cast<underlying_type>(val);
        bool success = read_value(underlying_value);
        if(success) {
            val = static_cast<T>(underlying_value);
            return true;
        }
        return false;
    }

    bool read_value(std::string& val) {
        std::size_t distance = std::distance(_begin, _end);
        if (distance < sizeof(std::uint32_t)) return false;
        std::uint32_t len;
        std::memcpy(&len, _begin, sizeof(len));
        _begin += sizeof(len);
        distance -= sizeof(len);
        if (distance < len) return false;
        val.assign(reinterpret_cast<const char*>(_begin), len);
        _begin += len;
        return true;
    }

    template <typename Clock, typename Duration>
    bool read_value(std::chrono::time_point<Clock, Duration>& tp) {
        Duration dur;
        if (!read_value(dur))
            return false;
        tp = std::chrono::time_point<Clock, Duration>(dur);
        return true;
    }

    template <typename T, typename Period>
    bool read_value(std::chrono::duration<T, Period>& dur) {
        T count;
        if (!read_value(count))
            return false;
        dur = std::chrono::duration<T, Period>(count);
        return true;
    }

    bool read_value(boost::uuids::uuid& uuid) {
        if (std::distance(_begin, _end) < 16) return false;
        std::memcpy(uuid.data(), _begin, 16);
        _begin += 16;
        return true;
    }

    bool read_value(boost::asio::ip::address& addr) {
        if (std::distance(_begin, _end) < 1) return false;
        uint8_t family = *_begin++;
        if (family == 0x04) {
            if (std::distance(_begin, _end) < 4) return false;
            boost::asio::ip::address_v4::bytes_type bytes;
            std::memcpy(bytes.data(), _begin, 4);
            _begin += 4;
            addr = boost::asio::ip::address_v4(bytes);
            return true;
        } else if (family == 0x06) {
            if (std::distance(_begin, _end) < 16) return false;
            boost::asio::ip::address_v6::bytes_type bytes;
            std::memcpy(bytes.data(), _begin, 16);
            _begin += 16;
            addr = boost::asio::ip::address_v6(bytes);
            return true;
        }
        return false;
    }

    pointer_type& _begin;
    const pointer_type  _end;
};

struct binary_size_calculator {
    explicit binary_size_calculator(std::size_t& size) : _byte_size(size) {}

    template <typename ParamT>
    void operator()(const ParamT& d) {
        const auto& v = d.value();
        using ValueType = typename ParamT::value_type;

        if constexpr (udho::utils::traits::is_optional<ValueType>::value) {
            if (v.has_value()) {
                _byte_size += 1;
                _byte_size += expected_size(v.value());
            } else {
                _byte_size += 1;
            }
        } else {
            // mandatory field: just write the value (no presence flag)
            _byte_size += expected_size(v);
        }
    }

private:
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    std::size_t expected_size(const T& val) {
        return sizeof(T);
    }

    template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    std::size_t expected_size(const T& val) {
        using underlying_type = std::underlying_type_t<T>;

        underlying_type underlying_value = static_cast<underlying_type>(val);
        return expected_size(underlying_value);
    }

    std::size_t expected_size(const std::string& val) {
        return sizeof(std::uint32_t) +val.size();
    }

    template <typename Clock, typename Duration>
    std::size_t expected_size(const std::chrono::time_point<Clock, Duration>& val) {
        return expected_size(val.time_since_epoch());
    }

    template <typename T, typename Ratio>
    std::size_t expected_size(const std::chrono::duration<T, Ratio>& val) {
        return expected_size(val.count());
    }

    std::size_t expected_size(const boost::uuids::uuid&) {
        return 16;
    }

    std::size_t expected_size(const boost::asio::ip::address& addr) {
        if (addr.is_v4()) return 1 + 4;    // family + 4 bytes
        else              return 1 + 16;   // family + 16 bytes
    }


private:
    std::size_t& _byte_size;
};

}


}
}

#endif // UDHO_LOGGING_SERIALIZATION_H
