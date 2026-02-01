#ifndef UDHO_URL_ROUTE_INDEX_H
#define UDHO_URL_ROUTE_INDEX_H

#include <string>

namespace udho{
namespace url{

namespace detail{

class route_index{
public:
    enum class type{
        none, action, registry
    };
private:
    type        _type;
    int         _mountpoint;
    int         _action;
    std::string _target;

public:
    inline route_index(const std::string& target, int mountpoint, int action): _type(type::action), _target(target), _mountpoint(mountpoint), _action(action) {}
    inline route_index(const std::string& target, route_index::type type = route_index::type::none): _type(type), _target(target), _mountpoint(-1), _action(-1) {}

    route_index(const route_index&) = default;
    inline route_index(route_index::type type, const route_index& other): _type(type), _mountpoint(other._mountpoint), _action(other._action), _target(other._target) {}
    inline route_index(route_index::type type, route_index&& other): _type(type), _mountpoint(other._mountpoint), _action(other._action), _target(std::move(other._target)) {}

    inline route_index::type type() const { return _type; }

    inline bool valid() const { return _type != type::none && _mountpoint >= 0 && _action >= 0; }

    inline int mountpoint() const { return _mountpoint; }
    inline int action() const { return _action; }
    inline const std::string target() const { return _target; }
};


}

}
}

#endif // UDHO_URL_ROUTE_INDEX_H
