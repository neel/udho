/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_VIEW_RESOURCES_FWD_H
#define UDHO_VIEW_RESOURCES_FWD_H

#include <filesystem>

/**
 * @defgroup resources Resources
 * @brief Resources Subsystem.
 * @{
 */
/** @} */

namespace udho{
namespace view{
namespace resources{

namespace tmpl{

template <typename BridgeT>
struct proxy;

}

namespace asset{

    namespace source{
        enum class type{
            none, memory, disk, remote
        };

        template <typename Iterator = const char*>
        struct memory{
            static constexpr type source = type::memory;
            using iterator = Iterator;
        };

        template <typename Path = std::filesystem::path>
        struct disk{
            static constexpr type source = type::disk;
            using path = Path;
        };

        struct remote{
            static constexpr type source = type::remote;
        };
    };

    enum class type{ none, js, css, txt, img };

    struct const_store;

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_FWD_H
