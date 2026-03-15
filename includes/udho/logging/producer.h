#ifndef UDHO_LOGGING_PRODUCER_H
#define UDHO_LOGGING_PRODUCER_H

#include <optional>
#include <udho/logging/ipc_queue.h>
#include <udho/logging/message.h>

namespace udho {
namespace logging {

struct producer{
    using message_type  = udho::logging::message;

    static bool log(const message_type& message) {
        if(_ipc_queue) {
            return (*_ipc_queue).try_send(message);
        } else {
            // discard
        }
        return false;
    }

    static void activate(const char* name = "") {
        if(!_ipc_queue) {
            _ipc_queue.emplace(boost::interprocess::open_only, name);
        }
    }

    static void deactivate() {
        if(_ipc_queue) {
            _ipc_queue.reset();
        }
    }

private:
    static std::optional<udho::logging::detail::ipc_queue> _ipc_queue;
};

std::optional<udho::logging::detail::ipc_queue> producer::_ipc_queue = std::nullopt;

}
}

#endif // UDHO_LOGGING_PRODUCER_H
