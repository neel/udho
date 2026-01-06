#ifndef UDHO_SESSION_ABSTRACT_CATALOGUE_H
#define UDHO_SESSION_ABSTRACT_CATALOGUE_H

#include <udho/session/fwd.h>
#include <udho/session/defs.h>
#include <udho/session/note.h>

namespace udho{
namespace session{

struct abstract_catalogue {
    using key_type  = udho::session::id;
    using note_type = note;

    inline explicit abstract_catalogue(udho::session::modes mode): _mode(mode) {}

    virtual note_type borrow(const key_type& sessid, bool expect_existing) = 0;
    virtual void remove(const key_type& sessid) = 0;
    virtual bool exists(const key_type& sessid) = 0;

    udho::session::modes mode() const { return _mode; }

private:
    udho::session::modes _mode;
};
}
}

#endif // UDHO_SESSION_ABSTRACT_CATALOGUE_H
