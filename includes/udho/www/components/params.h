#ifndef UDHO_WWW_SETTINGS_H
#define UDHO_WWW_SETTINGS_H

#include <udho/manifold/config.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{
namespace params{

namespace handler{}
namespace pg{}

namespace protocol{
    UDHO_CONFIG_PARAM(header_time_limit,      std::size_t,  30);     // maximum time spent (in seconds) for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(header_memory_limit,    std::size_t,  1024);   // maximum number of bytes that can be used for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(body_time_limit,        std::size_t,  60);     // maximum time spent (in seconds) for reading the body part of an HTTP request
    UDHO_CONFIG_PARAM(body_memory_limit,      std::size_t,  4096);   // maximum number of bytes allowed for the HTTP request body
    UDHO_CONFIG_PARAM(field_memory_limit,     std::size_t,  1024);   // maximum number of bytes allowed for a form field HTTP in the request body
    UDHO_CONFIG_PARAM(contiguous_buffer,      bool,         true);   // use flat_buffer if contiguous_buffer is true, otherwise use multi_buffer
}

namespace routing{
    UDHO_CONFIG_PARAM(use_trie,               bool,         false);
}

namespace cookies{
    UDHO_CONFIG_PARAM(enabled,                bool,         false);   // skip if not enabled
}

namespace session{
    UDHO_CONFIG_PARAM(enabled,                bool,         false);   // skip if not enabled
    UDHO_CONFIG_PARAM(sesskey,                std::string,  "sessid");
    UDHO_CONFIG_PARAM(domain,                 std::string,  "localhost");
    UDHO_CONFIG_PARAM(path,                   std::string,  "/");
}

namespace resources{
    UDHO_CONFIG_PARAM(enabled,                bool,         false);   // skip if not enabled
}


}
}
}

/** @} */

#endif // UDHO_WWW_SETTINGS_H
