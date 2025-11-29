#ifndef UDHO_MANIFOLD_EVALUATOR_helper_H
#define UDHO_MANIFOLD_EVALUATOR_helper_H

#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/journal.h>

namespace udho {
namespace manifold {


class exclusive_result{
    std::exception_ptr _exception;
public:
    exclusive_result() = default;
    exclusive_result(const exclusive_result&) = default;
    exclusive_result(exclusive_result&&) = default;
    exclusive_result(std::exception_ptr&& exptr): _exception(std::move(exptr)) {}
    exclusive_result& operator=(const exclusive_result&) = default;
public:
    exclusive_result& operator=(std::exception_ptr&& exptr) {
        _exception = std::move(exptr);
        return *this;
    }
public:
    bool operator()() const {
        if(_exception) {
            std::rethrow_exception(_exception);
        }
        return true;
    }
public:
    bool success() const { return !_exception; }
    bool error() const { return !success(); }
public:
    bool operator*() const { return success(); }
    void rethrow() const {
        assert(error());
        std::rethrow_exception(_exception);
    }
public:
    operator bool() const { return success(); }
    bool operator!() const { return error(); }
};

namespace detail {

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT, bool YieldsResult=udho::manifold::has_result<FacetT>::value>
class next_evaluator_helper_internal{
    using facet_type      = FacetT;
    using handler_type    = HandlerT;
    using args_tuple_type = ArgsTupleT;
    using result_type     = typename udho::manifold::facet_traits<facet_type>::result_type;
    using journal_type    = typename HandlerT::journal_type;

    handler_type _handler;
    args_tuple_type _args;
public:
    inline explicit next_evaluator_helper_internal(handler_type&& handler, args_tuple_type&& args): _handler(std::move(handler)), _args(std::move(args)) {}
    next_evaluator_helper_internal(const next_evaluator_helper_internal&) = delete;
    next_evaluator_helper_internal(next_evaluator_helper_internal&& other): _handler(std::move(other._handler)), _args(std::move(other._args)) {}

    void pass(result_type&& result) {
        _handler.journal().template get<facet_type>() = std::move(result);
        proceed();
    }

    void skip(){
        proceed();
    }

    void fail(result_type&& result){
        _handler.journal().template get<facet_type>() = std::move(result);
        _handler.completion()(false);
    }

    void fail(std::exception_ptr&& ex){
        _handler.completion()(std::move(ex));
    }

    void fail(std::exception&& ex){
        _handler.completion()(std::make_exception_ptr(std::move(ex)));
    }

    void proceed(){
        std::apply([&](auto&&... args){
            _handler.template operator()<Idx>(std::forward<decltype(args)>(args)...);
        }, _args);
    }

    void operator()(result_type&& result, bool success = true){
        if(success) pass(std::forward<result_type>(result));
        else        fail(std::forward<result_type>(result));
    }
};

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT>
class next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT, false>{
    using facet_type      = FacetT;
    using handler_type    = HandlerT;
    using args_tuple_type = ArgsTupleT;
    using result_type     = typename udho::manifold::facet_traits<facet_type>::result_type;
    using journal_type    = typename HandlerT::journal_type;

    handler_type _handler;
    args_tuple_type _args;
public:
    inline explicit next_evaluator_helper_internal(handler_type&& handler, args_tuple_type&& args): _handler(std::move(handler)), _args(std::move(args)) {}
    next_evaluator_helper_internal(const next_evaluator_helper_internal&) = delete;
    next_evaluator_helper_internal(next_evaluator_helper_internal&& other): _handler(std::move(other._handler)), _args(std::move(other._args)) {}

    void pass() {
        std::apply([&](auto&&... args){
            _handler.template operator()<Idx>(std::forward<decltype(args)>(args)...);
        }, _args);
    }

    void skip(){
        pass();
    }

    void fail(){
        _handler.completion()(false);
    }

    void fail(std::exception&& ex){
        _handler.completion()(std::make_exception_ptr(std::move(ex)));
    }

    void operator()(bool success = true){
        if(success) pass();
        else        fail();
    }
};

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT>
struct next_evaluator_helper: next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT>{
    using base_type = next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT>;

    using base_type::base_type;
};



template <std::size_t, typename...>
struct evaluator_helper;

/**
 * @brief The evaluator_helper class provides encapsulates the stage and an order of features and provides a handler as the inner class for facet evaluation.
 * @tparam Stage filter facets by the stage
 * @tparam Features... The ordered set of features by which the facets will be evaluated
 */
template <std::size_t Stage, typename FeatureX, typename... Features>
struct evaluator_helper<Stage, FeatureX, Features...>{

    /**
     * @brief The handler is used by detail::next_evaluator_helper_internal.
     * @tparam Facets... The facets
     * Contains a reference to the fabric and the journal for all the facets
     */
    template <typename... Facets>
    struct handler{
        using handler_type = handler<Facets...>;
        using fabric_type  = udho::manifold::fabric<Stage, Facets...>;
        using journal_type = typename udho::manifold::detail::journal_for_facets<Facets...>::type;
        using safe_success_type = std::variant<bool, std::exception_ptr>;
        using async_callback_type = std::function<void (safe_success_type)>;

        /**
         * @brief handler constructor
         * @param fabric
         * @param journal
         * @param callback
         *
         * @note The callback will be called with an instance of safe_success_type containing std::current_exception in case an exception
         *       is thrown by a facet in the fabric
         */
        handler(fabric_type& fabric, journal_type& journal, async_callback_type& callback): _fabric(fabric), _journal(journal), _callback(callback) {}
        handler(handler&& other): _fabric(other._fabric), _journal(other._journal), _callback(other._callback) {}
        handler(const handler&) = delete;

        journal_type& journal() { return _journal; }

        async_callback_type& completion() { return _callback; }

        /**
         * @brief skips evaluation of Idx'th facet offering feature FeatureX because it doesn't exist, then passes to the next feature
         */
        template <std::size_t Idx, typename... Args, std::enable_if_t<std::is_void_v<typename fabric_type::template facet_type<FeatureX, Idx>>, bool> = true>
        void eval(Args&&... args){
            using args_tuple_type   = std::tuple<Args&&...>;
            using rest_handler_type = typename evaluator_helper<Stage, Features...>::template handler<Facets...>;

            rest_handler_type rest_handler{_fabric, _journal, _callback};
            rest_handler.template operator()<0, Args...>(std::forward<Args>(args)...);
        }

        /**
         * @brief evaluates the Idx'th facet offering feature FeatureX, while passing the next_evaluator_helper pointing at the (Idx+1)'th facet offering the same feature
         * @pre fabric must contain atleast Idx+1 number of facets offering feature FeatureX for this function to be enabled
         */
        template <std::size_t Idx, typename... Args, std::enable_if_t<(fabric_type::template count<FeatureX>() > 0 && fabric_type::template count<FeatureX>()-1 > Idx), bool> = true>
        void eval(Args&&... args){
            using facet_type      = typename fabric_type::template facet_type<FeatureX, Idx>;
            using args_tuple_type = std::tuple<Args&&...>;
            using next_type       = next_evaluator_helper<Idx+1, args_tuple_type, facet_type, handler_type>;

            auto& wrapper = _fabric.template at<FeatureX, Idx>();
            auto& facet   = wrapper.facet();

            try{
                facet(
                    _journal,
                    next_type{std::move(*this), std::forward_as_tuple(args...)},
                    std::forward<Args>(args)...
                ); // facet will call the pass or fail method of the next_evaluator_helper
            } catch(...) {
                _callback(std::current_exception());
            }
        }

        /**
         * @brief evaluates the Idx'th facet offering feature FeatureX, while passing the next_evaluator_helper pointing at the facet offering the next feature
         * @pre fabric must contain exactly Idx+1 number of facets offering feature FeatureX for this function to be enabled
         */
        template <std::size_t Idx, typename... Args, std::enable_if_t<(fabric_type::template count<FeatureX>() > 0 && fabric_type::template count<FeatureX>()-1 == Idx), bool> = true>
        void eval(Args&&... args){
            using facet_type        = typename fabric_type::template facet_type<FeatureX, Idx>;
            using args_tuple_type   = std::tuple<Args&&...>;
            using rest_handler_type = typename evaluator_helper<Stage, Features...>::template handler<Facets...>;
            using next_type         = next_evaluator_helper<0, args_tuple_type, facet_type, rest_handler_type>;

            auto& wrapper = _fabric.template at<FeatureX, Idx>();
            auto& facet   = wrapper.facet();

            try{
                facet(
                    _journal,
                    next_type{rest_handler_type{_fabric, _journal, _callback}, std::forward_as_tuple(args...)},
                    std::forward<Args>(args)...
                ); // facet will call the pass or fail method of the next_evaluator_helper
            } catch(...) {
                _callback(std::current_exception());
            }
        }

        /**
         * @brief forwards the call to @ref eval<Idx>
         * @param args...
         * @note called by next_evaluator_helper for passing to the next facet
         * @details operator<0>() -> eval<0>(args...) resolve to either of the eval:{count<FeatureX>()-1 == Idx} or eval:{count<FeatureX>()-1 > Idx} depending of count<FeatureX>().
         *          In case of eval<0> these two options translate to either   eval:{count<FeatureX>() == 1}     or eval:{count<FeatureX>() > 1}.
         *          For the first case eval:{count<FeatureX>() == 1} the next_type is set for evaluation of Features...
         *          For the second case evaluation proceeds to eval<1>(args...) using the same feature through the next_evaluator_helper.
         *          Finally ot hits out_of_range causing substitution failure of both of these function, and enabling eval:{is_void_v<typename fabric_type::template facet_type<FeatureX, Idx>>, bool>}
         *          which doesn;t evaluate anything but proceeds to the next features.
         */
        template <std::size_t Idx, typename... Args>
        void operator()(Args&&... args){
            eval<Idx>(std::forward<Args>(args)...);
        }

        private:
            fabric_type&  _fabric;
            journal_type& _journal;
            async_callback_type& _callback;
    };


};

/**
 * @brief The evaluator_helper specialization with no feature, intended to be called in the end of evaluating all features unless one intermediate facet fails
 */
template <std::size_t Stage>
struct evaluator_helper<Stage>{
    template <typename... Facets>
    struct handler{
        using handler_type = handler<Facets...>;
        using fabric_type  = udho::manifold::fabric<Stage, Facets...>;
        using journal_type = typename udho::manifold::detail::journal_for_facets<Facets...>::type;
        using safe_success_type = std::variant<bool, std::exception_ptr>;
        using async_callback_type = std::function<void (safe_success_type)>;

        handler(fabric_type& fabric, journal_type& journal, async_callback_type& callback): _fabric(fabric), _journal(journal), _callback(callback) {}
        handler(handler&& other): _fabric(other._fabric), _journal(other._journal), _callback(other._callback) {}
        handler(const handler&) = delete;

        journal_type& journal() { return _journal; }

        async_callback_type& completion() { return _callback; }

        /**
         * @brief operator () calls the callback with true value because if the control reaches here it means that all the facets have passed.
         * @param args
         */
        template <std::size_t Idx, typename... Args>
        void operator()(Args&&... args){
            _callback(true);
        }

        private:
            fabric_type&  _fabric;
            journal_type& _journal;
            async_callback_type& _callback;
    };
};

}

}
}

#endif // UDHO_MANIFOLD_EVALUATOR_helper_H
