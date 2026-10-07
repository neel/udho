#ifndef UDHO_VIEW_RESOURCES_BLOBS_STORE_H
#define UDHO_VIEW_RESOURCES_BLOBS_STORE_H

#include <cstddef>
#include <map>
#include <string>
#include <udho/view/resources/blobs/blob.h>

namespace udho {
namespace view{
namespace resources {
namespace blobs {

class store {
public:
    using map_type = std::map<std::string, blob>;

    void view(const char* name, const unsigned char* bytes, std::size_t size) {
        _lua.emplace(name, blob{name, bytes, size});
    }

    void js(const char* name, const unsigned char* bytes, std::size_t size) {
        _js.emplace(name, blob{name, bytes, size});
    }

    void css(const char* name, const unsigned char* bytes, std::size_t size) {
        _css.emplace(name, blob{name, bytes, size});
    }

    void txt(const char* name, const unsigned char* bytes, std::size_t size) {
        _txt.emplace(name, blob{name, bytes, size});
    }

    void img(const char* name, const unsigned char* bytes, std::size_t size) {
        _img.emplace(name, blob{name, bytes, size});
    }

    const map_type& views() const { return _lua; }
    const map_type& js()    const { return _js;  }
    const map_type& css()   const { return _css; }
    const map_type& txt()   const { return _txt; }
    const map_type& imgs()  const { return _img; }

    blob view(const char* name) const { return get(_lua, name); }
    blob js  (const char* name) const { return get(_js,  name); }
    blob css (const char* name) const { return get(_css, name); }
    blob txt (const char* name) const { return get(_txt, name); }
    blob img (const char* name) const { return get(_img, name); }

private:
    static blob get(const map_type& m, const char* n) {
        static const unsigned char empty_data = 0;
        auto it = m.find(n);
        return it == m.end() ? blob{n, &empty_data, 0} : it->second;
    }

    map_type _lua, _js, _css, _txt, _img;
};


inline store& registry() {
    static store s;
    return s;
}

}
}
}
}


#endif // UDHO_VIEW_RESOURCES_BLOBS_STORE_H
