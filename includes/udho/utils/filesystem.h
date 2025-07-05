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

#ifndef UDHO_UTILS_FILESYSTEM_H
#define UDHO_UTILS_FILESYSTEM_H

#if defined(__cpp_lib_filesystem) && (__cpp_lib_filesystem >= 201703L)
    #include <filesystem>
    #define UDHO_INTERNAL_USING_STD_FILESYSTEM
#elif defined(__cpp_lib_experimental_filesystem)
    #include <experimental/filesystem>
    #define UDHO_INTERNAL_USING_STD_EXPERIMENTAL_FILESYSTEM
#else
    #if __cplusplus >= 201703L
        #include <filesystem>
        #define UDHO_INTERNAL_USING_STD_FILESYSTEM
    #elif __cplusplus >= 201402L
        #include <experimental/filesystem>
        #define UDHO_INTERNAL_USING_STD_EXPERIMENTAL_FILESYSTEM
    #else
        #include <boost/filesystem.hpp>
        #define UDHO_INTERNAL_USING_BOOST_FILESYSTEM
    #endif
#endif

namespace udho {
namespace utils {

#if defined(UDHO_INTERNAL_USING_STD_FILESYSTEM)
    namespace filesystem = std::filesystem;
#elif defined(UDHO_INTERNAL_USING_STD_EXPERIMENTAL_FILESYSTEM)
    namespace filesystem = std::experimental::filesystem;
#else
    namespace filesystem = boost::filesystem;
#endif

}
}

#endif // UDHO_UTILS_FILESYSTEM_H
