#ifndef UDHO_LOGGING_IPC_QUEUE_H
#define UDHO_LOGGING_IPC_QUEUE_H

#include <boost/interprocess/ipc/message_queue.hpp>
#include <udho/logging/message.h>
#include <array>

namespace udho {
namespace logging {

namespace detail {

struct ipc_queue {
    using size_type     = boost::interprocess::message_queue::size_type;
    using priority_type = std::uint32_t;
    using message_type  = udho::logging::message;

    static constexpr const char* default_ipcq_name = "udho-log-hiper";
    static constexpr size_type max_messages        = 1000;
    static constexpr size_type max_message_size    = 4 * 1024; // 4KB

    ipc_queue(const ipc_queue&) = delete;
    ipc_queue& operator=(const ipc_queue&) = delete;

    ipc_queue(ipc_queue&&) = delete;
    ipc_queue& operator=(ipc_queue&&) = delete;

    ~ipc_queue() = default;

    ipc_queue(boost::interprocess::open_only_t, const char* name = 0x0): _mq(boost::interprocess::open_only, name ? name : default_ipcq_name) {}
    ipc_queue(const char* name = 0x0): ipc_queue(boost::interprocess::open_only, name ? name : default_ipcq_name) {}

    static ipc_queue create(const char* name = 0x0) {
        ipc_queue::remove(name ? name : default_ipcq_name);
        return ipc_queue(boost::interprocess::create_only, name ? name : default_ipcq_name);
    }

    static void remove(const char* name = 0x0) {
        boost::interprocess::message_queue::remove(name ? name : default_ipcq_name);
    }

public:
    bool try_send(const message_type& message) {
        message_type::buffer_type buffer = message.save();
        udho::logging::severity severity = message[udho::logging::params::severity::val].value();
        static_assert(std::is_convertible_v<std::underlying_type_t<udho::logging::severity>, priority_type>);
        priority_type priority = static_cast<priority_type>(severity);
        return try_send(buffer.data(), buffer.size(), priority);
    }

    void send(const message_type& message) {
        message_type::buffer_type buffer = message.save();
        udho::logging::severity severity = message[udho::logging::params::severity::val].value();
        static_assert(std::is_convertible_v<std::underlying_type_t<udho::logging::severity>, priority_type>);
        priority_type priority = static_cast<priority_type>(severity);
        send(buffer.data(), buffer.size(), priority);
    }

    bool try_receive(message_type& message) {
        size_type size = 0;
        auto [success, priority] = try_receive(_read_buffer.data(), _read_buffer.size(), size);
        // priority intentionally unused because severity parameter already exists in the log message
        if(success) {
            return message.load(_read_buffer.data(), size);
        }
        return false;
    }

    bool receive(message_type& message) {
        size_type size = 0;
        priority_type priority = receive(_read_buffer.data(), _read_buffer.size(), size);
        // priority intentionally unused because severity parameter already exists in the log message
        return message.load(_read_buffer.data(), size);
    }

private:
    ipc_queue(boost::interprocess::create_only_t, const char* name = 0x0): _mq(boost::interprocess::create_only, name ? name : default_ipcq_name, max_messages, max_message_size) {}

private:

    bool try_send(const void* data, size_type size, priority_type priority = 0) {
        try {
            return _mq.try_send(data, size, priority);
        } catch (const boost::interprocess::interprocess_exception& e) {
            return false;
        }
    }


    void send(const void* data, size_type size, priority_type priority = 0) {
        _mq.send(data, size, priority);
    }

    std::pair<bool, std::uint32_t> try_receive(void* buffer, size_type buffer_size, size_type& received_size) {
        unsigned int priority = 0;
        if (_mq.try_receive(buffer, buffer_size, received_size, priority)) {
            return std::make_pair(true, priority);
        }
        return std::make_pair(false, priority);
    }

    std::uint32_t receive(void* buffer, size_type buffer_size, size_type& received_size) {
        unsigned int priority = 0;
        _mq.receive(buffer, buffer_size, received_size, priority);
        return priority;
    }

    // consumtion is single process, single threaded
    // therefore, it must be sequential
    // therefore, a single read buffer is sufficient
    // This read buffer will get rewritten upon each receive

    boost::interprocess::message_queue         _mq;
    std::array<std::uint8_t, max_message_size> _read_buffer;

};

}

}
}

#endif // UDHO_LOGGING_IPC_QUEUE_H
