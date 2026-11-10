#ifndef UDHO_WWW_RESPONSE_REGISTRY_H
#define UDHO_WWW_RESPONSE_REGISTRY_H

#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

#include <udho/logging/macros.h>
#include <udho/url/summary.h>

namespace udho {
namespace www {

/**
 * @brief Stream-neutral response completion state shared with action contexts.
 *
 * The registry stores one FIFO completion queue for each active flow. Queue
 * mutations are serialized by a single mutex, but callbacks and captured state
 * are invoked or destroyed only after that mutex has been released.
 */
class response_registry {
public:
    using hook_type = std::function<void ()>;
    using hooks_type = std::queue<hook_type>;

    explicit response_registry(const udho::url::summary::router& summary)
        : _summary(summary)
    {}

    response_registry(const response_registry&) = delete;
    response_registry& operator=(const response_registry&) = delete;

    /** @brief Register a flow whose response hooks may subsequently be added. */
    void attach(std::size_t flow_id) {
        const std::lock_guard<std::mutex> lock(_mutex);
        _hooks.try_emplace(flow_id);
    }

    /**
     * @brief Add a hook for the current response on an attached flow.
     * @return true when the flow exists; false when it has already detached.
     */
    bool add_hook(std::size_t flow_id, hook_type hook) {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto found = _hooks.find(flow_id);
        if(found == _hooks.end()) return false;

        if(hook) found->second.push(std::move(hook));
        return true;
    }

    /**
     * @brief Drain the current response's hooks while retaining the flow entry.
     *
     * Retaining the empty entry permits a keep-alive flow to register hooks for
     * its next response without another lifecycle attachment.
     */
    void complete(std::size_t flow_id) {
        hooks_type hooks;
        {
            const std::lock_guard<std::mutex> lock(_mutex);
            auto found = _hooks.find(flow_id);
            if(found == _hooks.end()) return;
            hooks.swap(found->second);
        }

        while(!hooks.empty()) {
            hook_type hook = std::move(hooks.front());
            hooks.pop();
            try {
                hook();
            } catch(const std::exception& ex) {
                UDHO_LOG_ERROR("www::response_registry", std::string{"Response finish hook threw an exception: "} + ex.what());
            } catch(...) {
                UDHO_LOG_ERROR("www::response_registry", "Response finish hook threw an unknown exception");
            }
        }
    }

    /**
     * @brief Remove a flow and discard any hooks it still owns.
     *
     * The queue is extracted first so destruction of hook captures happens
     * after releasing the registry mutex.
     */
    void detach(std::size_t flow_id) {
        hooks_type hooks;
        {
            const std::lock_guard<std::mutex> lock(_mutex);
            auto found = _hooks.find(flow_id);
            if(found == _hooks.end()) return;

            hooks.swap(found->second);
            _hooks.erase(found);
        }
    }

    /** @brief Return the immutable router summary associated with the runtime. */
    const udho::url::summary::router& summary() const noexcept {
        return _summary;
    }

private:
    std::unordered_map<std::size_t, hooks_type> _hooks;
    mutable std::mutex _mutex;
    udho::url::summary::router _summary;
};

} // namespace www
} // namespace udho

#endif // UDHO_WWW_RESPONSE_REGISTRY_H
