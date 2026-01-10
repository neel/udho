#ifndef UDHO_MANIFOLD_COMPONENTS_HANDLER_H
#define UDHO_MANIFOLD_COMPONENTS_HANDLER_H

#include <map>
#include <functional>
#include <udho/utils/format.h>
#include <udho/manifold/features.h>

namespace udho{
namespace manifold{

namespace components{

struct handler{
    using features = udho::manifold::features<>;
    static constexpr const char* name = "handler";

    using callback_type = std::function<void ()>;
    using collection_type = std::map<std::size_t, callback_type>;

    template <typename F>
    void add(std::size_t id, F&& callback){
        _handlers.emplace(id, callback_type{std::move(callback)});
    }

    void invoke(std::size_t id){
        auto it = _handlers.find(id);
        if(it != _handlers.end()) {
            callback_type callback = std::move(it->second);
            _handlers.erase(id);
            callback();
        } else {
            throw std::runtime_error(udho::utils::format("trying to invoke non-existent handler for flow {}", id));
        }
    }

private:
    collection_type _handlers;
};

}



}
}

#endif // UDHO_MANIFOLD_COMPONENTS_HANDLER_H
