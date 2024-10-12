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

#ifndef UDHO_VIEW_DATA_ASSOCIATIVE_H
#define UDHO_VIEW_DATA_ASSOCIATIVE_H

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/seq/seq.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/type.h>
#include <udho/view/data/detail/match.h>
#include <udho/view/data/detail/json.h>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

namespace detail{

template <typename Head, typename Tail = void>
struct associative;

/**
 * @brief A template structure for handling name-value pairs in an associative way, including recursion and JSON serialization (if nlohmann::json is available).
 *
 * @tparam PolicyT Type of the policy applied to the nvp (name-value pair).
 * @tparam KeyT Type of the key in the nvp.
 * @tparam ValueT Type of the value in the nvp.
 * @tparam Tail Type of the tail in a recursive structure. 'void' indicates the end of the recursion.
 */
template <typename PolicyT, typename KeyT, typename ValueT, typename Tail>
struct associative<data::nvp<PolicyT, KeyT, ValueT>, Tail>{
    using head_type = data::nvp<PolicyT, KeyT, ValueT>;
    using tail_type = Tail;

    template <typename Lhs, typename Rhs>
    friend struct concat_;

    template <typename DataT, typename AssociativeT>
    friend struct assign_;

    associative(head_type&& head, tail_type&& tail): _head(std::move(head)), _tail(std::move(tail)) {} ///< Constructs an associative with head and tail elements.

    /**
     * @brief Applies a function to each element until it returns true.
     *
     * @param f Function to apply.
     * @param count Initial count, defaults to 0.
     * @return std::size_t value representing the count of invocations until the function returns true.
     */
    template <typename Function>
    std::size_t apply_until(Function& f, std::size_t count = 0){
        bool res = f(_head);
        if(!res){
            return _tail.apply_until(f, count +1);
        } else {
            return count;
        }
    }

    /**
     * @brief Applies a function to each element that matches a given condition.
     *
     * @param f Function to apply.
     * @param match Function that determines whether the nvp matches the condition.
     * @param count Initial count, defaults to 0.
     * @return std::size_t value representing the number of invocations.
     */
    template <typename Function, typename MatchT>
    std::size_t apply_if(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
        }
        return _tail.apply_if(std::forward<Function>(f), std::forward<MatchT>(match), count +1);
    }

    /**
     * @brief Applies a function to all elements.
     *
     * @param f Function to apply.
     * @return std::size_t value representing the number of invocations.
     */
    template <typename Function>
    std::size_t apply_all(Function&& f){
        return apply_if(std::forward<Function>(f), detail::match_all{});
    }


#ifdef WITH_JSON_NLOHMANN

    /**
     * @brief Serializes the data into JSON format.
     *
     * @param data Data to serialize.
     * @return nlohmann::json object representing the serialized data.
     */
    template <typename DataT>
    nlohmann::json json(const DataT& data) {
        nlohmann::json root = nlohmann::json::object();
        detail::to_json_f jsonifier{root, data};
        apply_if(jsonifier, detail::match_all{});
        return root;
    }

    /**
     * @brief Deserializes JSON data into the object.
     *
     * @param data Object to fill with deserialized data.
     * @param json JSON object representing the data.
     */
    template <typename DataT>
    void json(DataT& data, const nlohmann::json& json) {
        detail::from_json_f jsonifier{json, data};
        apply_if(jsonifier, detail::match_all{});
    }

#endif

    private:
        head_type _head;
        tail_type _tail;
};


/**
 * @brief Specialization of the associative structure for when there is no tail element (tail is void).
 *
 * @tparam PolicyT Type of the policy applied to the nvp.
 * @tparam KeyT Type of the key in the nvp.
 * @tparam ValueT Type of the value in the nvp.
 */
template <typename PolicyT, typename KeyT, typename ValueT>
struct associative<data::nvp<PolicyT, KeyT, ValueT>, void>{
    using head_type = data::nvp<PolicyT, KeyT, ValueT>;
    using tail_type = void;

    template <typename Lhs, typename Rhs>
    friend struct concat_;

    template <typename DataT, typename AssociativeT>
    friend struct assign_;

    associative(head_type&& head): _head(std::move(head)) {}

    template <typename Function>
    std::size_t apply_until(Function& f, std::size_t count = 0){
        bool res = f(_head);
        if(!res){
            return count;
        } else {
            return count +1;
        }
    }

    template <typename Function, typename MatchT>
    std::size_t apply_if(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
            return count +1;
        }
        return count;
    }

    template <typename Function>
    std::size_t apply_all(Function&& f){
        return apply_if(std::forward<Function>(f), detail::match_all{});
    }


#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    nlohmann::json json(const DataT& data) {
        nlohmann::json root = nlohmann::json::object();
        detail::to_json_f jsonifier{root, data};
        apply_if(jsonifier, detail::match_all{});
        return root;
    }

    template <typename DataT>
    void json(DataT& data, const nlohmann::json& json) {
        detail::from_json_f jsonifier{json, data};
        apply_if(jsonifier, detail::match_all{});
    }

#endif

    private:
        head_type _head;
};

template <typename MembersT = void>
struct assoc_;

template <>
struct assoc_<void>{
    assoc_(const std::string& name): _name(name) {}
    const std::string& name() const { return _name; }
    private:
        std::string  _name;
};

template <typename HeadT, typename TailT>
struct assoc_<associative<HeadT, TailT>>{
    using members_type = associative<HeadT, TailT>;

    assoc_(const std::string& name, members_type&& members): _name(name), _members(std::move(members)) {}
    const std::string& name() const { return _name; }

    members_type& members() { return _members; }
    const members_type& members() const { return _members; }

    private:
        std::string  _name;
        members_type _members;
};

template <typename DataT, typename AssociativeT>
struct assign_;

/**
 * @brief assigns i'th item in the associative container
 */
template <typename DataT, typename HeadT, typename TailT>
struct assign_<DataT, associative<HeadT, TailT>>{

    template <typename OtherDataT, typename AssociativeT>
    friend struct assign_;

    /**
     * @brief Assigns i'th item in the associative container
     * @param assoc reference to the container
     * @param idx index of the target nvp
     */
    assign_(DataT& data, associative<HeadT, TailT>& assoc, std::size_t idx = 0): _data(data), _assoc(assoc), _idx(idx) {}

    std::size_t apply_str(const std::string& str){
        return apply_str(str, 0);
    }

    private:
        /**
         * @brief assigns a string value by lexically converting it to appropriate value of the nvp. Skips readonly nvps. Hence the requested `target_index` may not match with the infected index
         * @return returns infected index which is the index of the nvp that has been modified +1. Hence if nothing is modified returns the value of target index which was passed to the constructor.
         */
        std::size_t apply_str(const std::string& str, std::size_t c){
            assert(c <= _idx);

            std::size_t count = c;
            bool success = (count == _idx) && apply_str(_assoc._head, str);
            if(!success){
                assign_<DataT, TailT> assign{_data, _assoc._tail, _idx};
                return assign.apply_str(str, count+1);
            } else {
                return count +1;
            }
        }

        template <typename PolicyT, typename KeyT, typename ValueT, std::enable_if_t<data::policies::is_writable_property_v<PolicyT>, int >* = nullptr>
        bool apply_str(data::nvp<PolicyT, KeyT, ValueT>& head, const std::string& v){
            bool okay = false;
            std::decay_t<typename ValueT::value_type> input = udho::url::detail::convert_str_to_type<std::decay_t<typename ValueT::value_type>>::apply(v, &okay);
            if (okay) {
                if(!head.value().set(_data, input)){
                    throw std::invalid_argument{udho::url::format("Failed to set argument {} for nvp {}", v, head.name())};
                }
                return true;
            } else {
                throw std::invalid_argument{udho::url::format("Failed to lexically convert argument {} for nvp {}", v, head.name())};
            }
            return okay;
        }

        template <typename PolicyT, typename KeyT, typename ValueT, std::enable_if_t<!data::policies::is_writable_property_v<PolicyT>, int >* = nullptr>
        bool apply_str(data::nvp<PolicyT, KeyT, ValueT>& head, const std::string& v){ return false; }

    private:
        DataT&                     _data;
        associative<HeadT, TailT>& _assoc;
        std::size_t                _idx;
};

template <typename DataT, typename HeadT>
struct assign_<DataT, associative<HeadT, void>>{

    template <typename OtherDataT, typename AssociativeT>
    friend struct assign_;

    /**
     * @brief Assigns i'th item in the associative container
     * @param assoc reference to the container
     * @param idx index of the target nvp
     */
    assign_(DataT& data, associative<HeadT, void>& assoc, std::size_t idx = 0): _data(data), _assoc(assoc), _idx(idx) {}

    std::size_t apply_str(const std::string& str){
        return apply_str(str, 0);
    }

    private:

        /**
         * @brief assigns a string value by lexically converting it to appropriate value of the nvp. Skips readonly nvps. Hence the requested `target_index` may not match with the infected index
         * @return returns infected index which is the index of the nvp that has been modified +1. Hence if nothing is modified returns the value of target index which was passed to the constructor.
         */
        std::size_t apply_str(const std::string& str, std::size_t c = 0){
            assert(c <= _idx);

            std::size_t count = c;
            bool success = (count == _idx) && apply_str(_assoc._head, str);
            if(!success){
                return count;
            } else {
                return count +1;
            }
        }

        template <typename PolicyT, typename KeyT, typename ValueT, std::enable_if_t<data::policies::is_writable_property_v<PolicyT>, int >* = nullptr>
        bool apply_str(data::nvp<PolicyT, KeyT, ValueT>& head, const std::string& v){
            bool okay = false;
            std::decay_t<typename ValueT::value_type> input = udho::url::detail::convert_str_to_type<std::decay_t<typename ValueT::value_type>>::apply(v, &okay);
            if (okay) {
                if(!head.value().set(_data, input)){
                    throw std::invalid_argument{udho::url::format("Failed to set argument {} for nvp {}", v, head.name())};
                }
                return true;
            } else {
                throw std::invalid_argument{udho::url::format("Failed to lexically convert argument {} for nvp {}", v, head.name())};
            }
            return okay;
        }

        template <typename PolicyT, typename KeyT, typename ValueT, std::enable_if_t<!data::policies::is_writable_property_v<PolicyT>, int >* = nullptr>
        bool apply_str(data::nvp<PolicyT, KeyT, ValueT>& head, const std::string& v){ return false; }

    private:
        DataT&                     _data;
        associative<HeadT, void>&  _assoc;
        std::size_t                _idx;
};

/**
 * @brief assigns values to the nvp's of an associative container
 *
 * @param data  the targeted data object
 * @param assoc the associative container
 * @param begin iterator to a string container
 * @param end   iterator to a string container
 */
template <typename DataT, typename HeadT, typename TailT, typename IteratorT>
std::size_t assign(DataT& data, associative<HeadT, TailT>& assoc, IteratorT begin, IteratorT end){
    using assigner_type = assign_<DataT, associative<HeadT, TailT>>;

    if(std::distance(begin, end) == 0){
        return 0;
    }

    IteratorT i = begin;
    std::size_t assign_at = 0;
    std::size_t assignment_count = 0;
    do {
        assigner_type assigner{data, assoc, assign_at};
        std::string str = *i++;
        assign_at = assigner.apply_str(str);
        ++assignment_count;
    } while(i < end);

    return assignment_count;
}

}

inline detail::assoc_<> assoc(const std::string& name){ return detail::assoc_<>{name}; }

/**
 * @brief assigns values to the nvp's of an associative container
 *
 * @param data  the targeted data object
 * @param assoc the associative container
 * @param begin iterator to a string container
 * @param end   iterator to a string container
 */
template <typename DataT, typename IteratorT, typename std::enable_if<udho::view::data::has_metatype<DataT>::value, int>::type* = nullptr>
std::size_t assign(DataT& data, IteratorT begin, IteratorT end){
    auto assoc = metatype(udho::view::data::type<DataT>{});

    using assoc_type    = decltype(assoc);
    using members_type  = typename assoc_type::members_type;

    return detail::assign(data, assoc.members(), begin, end);
}

template <typename DataT, typename IteratorT, typename std::enable_if<!udho::view::data::has_metatype<DataT>::value, int>::type* = nullptr>
std::size_t assign(DataT&, IteratorT, IteratorT){ return 0; }

}
}
}

#endif // UDHO_VIEW_DATA_ASSOCIATIVE_H
