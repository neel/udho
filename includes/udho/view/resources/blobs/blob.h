#ifndef UDHO_VIEW_RESOURCES_BLOBS_BLOB_H
#define UDHO_VIEW_RESOURCES_BLOBS_BLOB_H

#include <cstddef>

namespace udho {
namespace view{
namespace resources {
namespace blobs {

struct blob {
    using bytes_type = const unsigned char*;

    blob(const char* name, bytes_type bytes, std::size_t size): _name(name), _bytes(bytes), _size(size) {}

    const char* name() const { return _name; }
    const unsigned char* begin() const { return _bytes; }
    const unsigned char* end() const { return _bytes + size(); }
    std::size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

private:
    const char* _name;
    bytes_type  _bytes;
    std::size_t _size;
};

}
}
}
}

#endif // UDHO_VIEW_RESOURCES_BLOBS_BLOB_H
