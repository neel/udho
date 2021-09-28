/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
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

#ifndef UDHO_ACTIVITIES_DATASET_H
#define UDHO_ACTIVITIES_DATASET_H

#include <string>
#include <memory>
#include <udho/cache.h>
#include <udho/activities/detail.h>
#include <udho/activities/fwd.h>

namespace udho{
/**
 * @ingroup activities
 */
namespace activities{

/**
 * @brief Hosts result_data for all activities in the activity chain.
 * - Creates an on-memory store using @ref udho::cache::store to contain the result data of all activities in the chains
 * - Multiple activities may yield same type of result_data, which is avoided by labeling the result_type with the activity_type
 * 
 * @see udho::cache::store
 * @see udho::cache::shadow
 * @ingroup data
 * @tparam T ... Activities in the chains
 */
template <typename... T>
struct dataset: detail::fixed_key_accessor<udho::cache::shadow<std::string, detail::labeled<T, typename T::result_type>...>>{
    typedef detail::fixed_key_accessor<udho::cache::shadow<std::string, detail::labeled<T, typename T::result_type>...>> base_type;
    typedef udho::cache::store<udho::cache::storage::memory, std::string, detail::labeled<T, typename T::result_type>...> store_type;
    typedef typename store_type::shadow_type shadow_type;
    // typedef accessor<T...> accessor_type;
    
    /**
     * @brief Construct a new dataset object
     * 
     * @param config udho::configuration_type 
     * @param name std::string
     */
    dataset(const udho::configuration_type& config, const std::string& name): base_type(_shadow, name), _store(config), _shadow(_store), _name(name){}
    /**
     * @brief Name of the dataset
     * 
     * @return std::string 
     */
    std::string name() const{ return _name; }
    /**
     * @brief Get a shadow of the store
     * 
     * @return udho::cache::shadow 
     */
    shadow_type& shadow() { return _shadow; }
    /**
     * @brief Get a shadow of the store
     * 
     * @return udho::cache::shadow 
     */
    const shadow_type& shadow() const { return _shadow; }

    private:
        store_type   _store;
        shadow_type  _shadow;
        std::string  _name;
};
    
}
}
#endif // UDHO_ACTIVITIES_DATASET_H
