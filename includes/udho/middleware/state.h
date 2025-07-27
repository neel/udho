#ifndef UDHO_MIDDLEWARE_STATE_H
#define UDHO_MIDDLEWARE_STATE_H

#include <type_traits>
#include <udho/middleware/features.h>
#include <udho/middleware/component.h>

namespace udho {
namespace middleware {

#ifndef __DOXYGEN__

/**
 * @brief The state_wrapper class
 * @tparam StateT
 * @note expects StateT to be movable (it is default constructibility and copy constructibility is not necessary)
 */
template <typename StateT, typename Feature>
struct state_wrapper{
    using type      = StateT;
    using feature   = Feature;
    using opt_type  = std::optional<type>;

    state_wrapper(): _state(std::nullopt) {}
    explicit state_wrapper(type&& state): _state(std::move(state)) {}

    state_wrapper(state_wrapper&&) = default;
    state_wrapper& operator=(state_wrapper&&) = default;

    state_wrapper& operator=(type&& state) {
        _state  = std::move(state);
        return *this;
    }

    bool ready() const { return _state.has_value(); }

    const type& value() const {
        if(!ready()) throw std::runtime_error{"trying to get value from unevaluated state"};
        return *_state;
    }

    type& value() {
        if(!ready()) throw std::runtime_error{"trying to get value from unevaluated state"};
        return *_state;
    }

    operator type() const { return value(); }

    const type& operator*() const { return value(); }
    type& operator*() { return value(); }

    const type* operator->() const { return &value(); }
    type* operator->() { return &value(); }

    operator bool() const { return ready(); }

private:
    opt_type _state;
};

template <typename ComponentT, typename... Tail>
struct states: private states<Tail...> {
    using state_type    = state_wrapper<typename ComponentT::state, typename ComponentT::feature>;

    template <typename HeadQ>
    using result_type = std::conditional_t<std::is_same_v<HeadQ, ComponentT>, state_type, typename states<Tail...>::template result_type<HeadQ>>;

    template <typename F>
    using result_type_by_feature = std::conditional_t<std::is_same_v<typename state_type::feature, F>, state_type, typename states<Tail...>::template result_type_by_feature<F>>;


    template <typename FeatureX, typename... Features>
    friend struct evaluator;

    inline explicit states() {}

    template <typename OtherHeadT, typename... OtherTail>
    inline explicit states(const states<OtherHeadT, OtherTail...>& other): _state(other.template get<ComponentT>()), states<Tail...>(other) {}


    template <typename HeadQ, std::enable_if_t<std::is_same_v<HeadQ, ComponentT>, bool> = true>
    state_type& get() { return _state; }

    template <typename HeadQ, std::enable_if_t<std::is_same_v<HeadQ, ComponentT>, bool> = true>
    const state_type& get() const { return _state; }

    template <typename HeadQ, std::enable_if_t<!std::is_same_v<HeadQ, ComponentT>, bool> = true>
    result_type<HeadQ>& get() { return states<Tail...>::template get<HeadQ>(); }

    template <typename HeadQ, std::enable_if_t<!std::is_same_v<HeadQ, ComponentT>, bool> = true>
    const result_type<HeadQ>& get() const { return states<Tail...>::template get<HeadQ>(); }


    template <typename FeatureT, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return 1+facade_chain<Tail...>::template count<FeatureT>(); }

    template <typename FeatureT, std::enable_if_t<!std::is_same_v<typename state_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return facade_chain<Tail...>::template count<FeatureT>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT> && Idx == 0, bool> = true>
    state_type& get() { return _state; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT> && Idx == 0, bool> = true>
    const state_type& get() const { return _state; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename state_type::feature, FeatureT>, bool> = true>
    result_type_by_feature<FeatureT>& get() { return facade_chain<Tail...>::template get<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename state_type::feature, FeatureT>, bool> = true>
    const result_type_by_feature<FeatureT>& get() const { return facade_chain<Tail...>::template get<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT> && Idx != 0, bool> = true>
    result_type_by_feature<FeatureT>& get() { return facade_chain<Tail...>::template get<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT> && Idx != 0, bool> = true>
    const result_type_by_feature<FeatureT>& get() const { return facade_chain<Tail...>::template get<FeatureT, Idx-1>(); }

private:
    states<Tail...>& tail() { return *this; }
    const states<Tail...>& tail() const { return *this; }

private:
    state_type _state;
};

template <typename ComponentT>
struct states<ComponentT> {
    using state_type    = state_wrapper<typename ComponentT::state, typename ComponentT::feature>;

    template <typename HeadQ>
    using result_type = std::enable_if_t<std::is_same_v<HeadQ, ComponentT>, state_type>;

    template <typename FeatureT>
    using result_type_by_feature = std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT>, state_type>;


    template <typename FeatureX, typename... Features>
    friend struct evaluator;


    inline explicit states() {}

    template <typename OtherHeadT, typename... OtherTail>
    inline explicit states(const states<OtherHeadT, OtherTail...>& other): state_type(other.template get<ComponentT>()) {}

    template <typename HeadQ, std::enable_if_t<std::is_same_v<HeadQ, ComponentT>, bool> = true>
    state_type& get() { return _state; }

    template <typename HeadQ, std::enable_if_t<std::is_same_v<HeadQ, ComponentT>, bool> = true>
    const state_type& get() const { return _state; }


    template <typename FeatureT, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return 1; }

    template <typename FeatureT, std::enable_if_t<!std::is_same_v<typename state_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return 0; }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT> && Idx == 0, bool> = true>
    state_type& get() { return _state; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename state_type::feature, FeatureT> && Idx == 0, bool> = true>
    const state_type& get() const { return _state; }

private:
    state_type _state;
};

#else


#endif // __DOXYGEN__


}
}


#endif // UDHO_MIDDLEWARE_STATE_H
