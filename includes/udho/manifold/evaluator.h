#ifndef UDHO_MANIFOLD_EVALUATOR_HELPER_H
#define UDHO_MANIFOLD_EVALUATOR_HELPER_H

#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/journal.h>

namespace udho {
namespace manifold {

/**
 * @addtogroup manifold
 * @{
 */


/**
 * @brief A wrapper for storing either a successful result or an exception
 *
 * This class encapsulates the result of an asynchronous operation that can
 * either succeed (returning true) or fail (storing an exception). It provides
 * a unified interface for checking success and rethrowing exceptions.
 *
 * @note Designed to be used in pipeline evaluation where exceptions need to
 *       be propagated across asynchronous boundaries.
 */
class exclusive_result{
    std::exception_ptr _exception;
    bool _success;
public:
    exclusive_result(): _success(false) {}                      ///< Default Constructor
    exclusive_result(const exclusive_result&) = default;        ///< Copy constructor
    exclusive_result(exclusive_result&&) = default;             ///< Move constructor
    exclusive_result& operator=(const exclusive_result&) = default;    ///< Copy assignment operator

    /// @brief Construct with an exception
    /// @param exptr Exception pointer to store
    exclusive_result(std::exception_ptr&& exptr): _exception(std::move(exptr)), _success(false) {}

    exclusive_result(bool success): _exception(nullptr), _success(success) {}

public:
    /// @brief Assign an exception
    /// @param exptr Exception pointer to store
    /// @return Reference to this object
    exclusive_result& operator=(std::exception_ptr&& exptr) {
        _exception = std::move(exptr);
        _success   = false;
        return *this;
    }

    /// @brief Assign an exception
    /// @param exptr Exception pointer to store
    /// @return Reference to this object
    exclusive_result& operator=(bool success) {
        _success = success;
        if(_success) {
            _exception = nullptr;
        }
        return *this;
    }
public:
    /// @brief Check and propagate exception
    /// @return Always returns true if no exception stored
    /// @throws The stored exception if one exists
    bool operator()() const {
        if(_exception) {
            std::rethrow_exception(_exception);
        }
        return true;
    }
public:
    bool value() const { return _success; }
    bool has_exception() const { return !!_exception; }
public:
    /// @brief Check if operation was successful
    /// @return true if no exception stored
    bool success() const { return !_exception && _success; }
    /// @brief Check if operation failed
    /// @return true if an exception is stored
    bool error() const { return !success(); }
public:
    /// @brief Dereference operator for checking success
    /// @return true if no exception stored
    bool operator*() const { return success(); }
    /// @brief Rethrow the stored exception
    /// @pre error() must be true
    void rethrow() const {
        assert(has_exception());
        std::rethrow_exception(_exception);
    }
public:
    /// @brief Boolean conversion for checking success
    /// @return true if no exception stored
    operator bool() const { return success(); }
    /// @brief Negation operator for checking failure
    /// @return true if an exception is stored
    bool operator!() const { return error(); }
};

namespace detail {

/**
 * @brief Internal helper for facet evaluation with result propagation
 *
 * This class manages the continuation of facet evaluation in a pipeline.
 *
 * @tparam Idx Index of the current facet in its feature group
 * @tparam ArgsTupleT Type of tuple storing arguments to forward
 * @tparam FacetT Type of the facet being evaluated
 * @tparam HandlerT Type of the handler managing evaluation state
 * @tparam YieldsResult Whether the facet produces a result (deduced from FacetT)
 *
 * @note This is an implementation detail and not intended for direct use.
 *
 * @see next_evaluator_helper
 * @see evaluator_helper
 */
template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT, bool YieldsResult=udho::manifold::has_result<FacetT>::value>
class next_evaluator_helper_internal{
    using facet_type      = FacetT;
    using handler_type    = HandlerT;
    using args_tuple_type = ArgsTupleT;
    using result_type     = typename udho::manifold::facet_traits<facet_type>::result_type;
    using journal_type    = typename HandlerT::journal_type;

    handler_type    _handler;
    args_tuple_type _args;
    bool            _done;
public:

    /**
     * @brief Construct with handler and arguments
     * @param handler Handler managing the evaluation state
     * @param args Arguments to forward to next evaluator
     */
    inline explicit next_evaluator_helper_internal(handler_type&& handler, args_tuple_type&& args): _handler(std::move(handler)), _args(std::move(args)), _done(false) {}

    next_evaluator_helper_internal(const next_evaluator_helper_internal&) = delete;

    /// @brief Move constructor
    next_evaluator_helper_internal(next_evaluator_helper_internal&& other): _handler(std::move(other._handler)), _args(std::move(other._args)), _done(other._done) {}

    /**
     * @brief Pass proceed to the next evaluator with a successful result
     * @param result The result to store in the journal
     *
     * Stores the result in the journal and proceeds to evaluate the next
     * facet in the pipeline.
     */
    void pass(result_type&& result) {
        if(_done) return;

        _done = true;
        _handler.journal().template get<facet_type>() = std::move(result);
        proceed();
    }

    /**
     * @brief proceed to the next evaluator without result
     *
     * Used when a facet doesn't need to be evaluated but the pipeline
     * should continue.
     */
    void skip(){
        if(_done) return;

        _done = true;
        proceed();
    }

    /**
     * @brief Fail with a result
     * @param result The failure result to store
     *
     * Stores the failure result in the journal and calls the completion
     * handler with false, which may terminate the pipeline
     */
    void fail(result_type&& result){
        if(_done) return;

        _done = true;
        _handler.journal().template get<facet_type>() = std::move(result);
        _handler.completion()(false);
    }

    /**
     * @brief Fail with an exception
     * @param exptr Exception pointer to propagate
     *
     * Terminates the pipeline by calling the completion handler with
     * the exception.
     */
    void fail(std::exception_ptr&& ex){
        if(_done) return;

        _done = true;
        _handler.completion()(std::move(ex));
    }

    /**
     * @brief Fail with an exception object
     * @param ex Exception to propagate
     *
     * Converts the exception to a std::exception_ptr and terminates
     * the pipeline.
     */
    template <typename ExceptionT, std::enable_if_t<std::is_base_of<std::exception, ExceptionT>::value, bool> = true>
    void fail(ExceptionT&& ex){
        if(_done) return;

        _done = true;
        _handler.completion()(std::make_exception_ptr(std::move(ex)));
    }

    /**
     * @brief Handle facet evaluation result
     * @param result The result from facet evaluation
     * @param success Whether the evaluation was successful
     *
     * Routes the result to either pass() or fail() based on success flag.
     */
    void operator()(result_type&& result, bool success = true){
        if(success) pass(std::forward<result_type>(result));
        else        fail(std::forward<result_type>(result));
    }

private:
    /**
     * @brief Proceed to the next facet
     *
     * Invokes the next evaluator in the pipeline with the stored arguments.
     */
    void proceed(){
        std::cout << "next_evaluator_helper_internal<" << Idx << ", ArgsTuple, " << facet_name<FacetT>::get() << ", HandlerT, " << YieldsResult << ">::proceed()" << std::endl;
        std::apply([&](auto&&... args){
            _handler.template operator()<Idx>(std::forward<decltype(args)>(args)...);
        }, _args);
    }
};


/**
 * @brief Specialization for facets that don't produce results
 *
 * This specialization handles facets that perform side effects but don't
 * return results to be stored in the journal.
 *
 * @tparam Idx Index of the current facet in its feature group
 * @tparam ArgsTupleT Type of tuple storing arguments to forward
 * @tparam FacetT Type of the facet being evaluated
 * @tparam HandlerT Type of the handler managing evaluation state
 */
template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT>
class next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT, false>{
    using facet_type      = FacetT;
    using handler_type    = HandlerT;
    using args_tuple_type = ArgsTupleT;
    using result_type     = typename udho::manifold::facet_traits<facet_type>::result_type;
    using journal_type    = typename HandlerT::journal_type;

    handler_type    _handler;
    args_tuple_type _args;
    bool            _done;
public:

    /**
     * @brief Construct with handler and arguments
     * @param handler Handler managing the evaluation state
     * @param args Arguments to forward to next evaluator
     */
    inline explicit next_evaluator_helper_internal(handler_type&& handler, args_tuple_type&& args): _handler(std::move(handler)), _args(std::move(args)), _done(false) {}
    next_evaluator_helper_internal(const next_evaluator_helper_internal&) = delete;
    next_evaluator_helper_internal(next_evaluator_helper_internal&& other): _handler(std::move(other._handler)), _args(std::move(other._args)), _done(other._done) {}

    /**
     * @brief Pass to the next facet
     *
     * Proceeds to evaluate the next facet in the pipeline without
     * storing any result.
     */
    void pass() {
        if(_done) return;

        _done = true;
        std::cout << "next_evaluator_helper_internal<" << Idx << ", ArgsTuple, " << facet_name<FacetT>::get() << ", HandlerT, " << false << ">::pass()" << std::endl;
        std::apply([&](auto&&... args){
            _handler.template operator()<Idx>(std::forward<decltype(args)>(args)...);
        }, _args);
    }

    /**
     * @brief Skip evaluation of this facet
     *
     * Equivalent to pass() for facets without results.
     */
    void skip(){
        if(_done) return;

        _done = true;
        pass();
    }

    /**
     * @brief Fail without a result
     *
     * Terminates the pipeline by calling the completion handler with false.
     */
    void fail(){
        if(_done) return;

        _done = true;
        _handler.completion()(false);
    }

    /**
     * @brief Fail with an exception object
     * @param ex Exception to propagate
     *
     * Converts the exception to a std::exception_ptr and terminates
     * the pipeline.
     */
    void fail(std::exception&& ex){
        if(_done) return;

        _done = true;
        _handler.completion()(std::make_exception_ptr(std::move(ex)));
    }

    /**
     * @brief Handle facet evaluation completion
     * @param success Whether the evaluation was successful
     *
     * Routes to either pass() or fail() based on success flag.
     */
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

/**
 * @brief Primary template for feature-based evaluator with at least one feature
 *
 * Provides the infrastructure for evaluating facets in a predefined order based on
 * on their features. It contains an inner handler class that manages the evaluation state.
 *
 * @tparam Stage Filter facets by the stage
 * @tparam FeatureX The current feature being evaluated
 * @tparam Features... The remaining features to evaluate
 */
template <std::size_t Stage, typename FeatureX, typename... Features>
struct evaluator_helper<Stage, FeatureX, Features...>{

    /**
     * @brief Manages facet evaluation state for a specific feature sequence
     *
     * This inner class maintains references to the fabric, journal, and
     * callback, and provides methods to evaluate facets in the correct order.
     *
     * @tparam JournalT Type of the journal storing facet results
     * @tparam Facets... The facets available in the fabric
     */
    template <typename JournalT, typename... Facets>
    struct handler{
        using handler_type          = handler<JournalT, Facets...>;
        using fabric_type           = udho::manifold::fabric<Stage, Facets...>;
        using journal_type          = JournalT; // typename udho::manifold::detail::journal_for_facets<Facets...>::type;
        using safe_success_type     = exclusive_result;
        using async_callback_type   = std::function<void (safe_success_type)>;

        /**
         * @brief Construct a handler
         * @param fabric Reference to the fabric containing facets
         * @param journal Reference to the journal for storing results
         * @param callback Callback to invoke on pipeline completion
         *
         * @note The callback will be called with an instance of safe_success_type
         *       containing std::current_exception in case an exception is thrown
         *       by a facet in the fabric
         */
        handler(fabric_type& fabric, journal_type& journal, async_callback_type& callback): _fabric(fabric), _journal(journal), _callback(callback) {}
        /// @brief Move constructor
        handler(handler&& other): _fabric(other._fabric), _journal(other._journal), _callback(other._callback) {}
        handler(const handler&) = delete;

        /// @brief Access the journal
        /// @return Reference to the journal
        journal_type& journal() { return _journal; }

        /// @brief Access the completion callback
        /// @return Reference to the callback
        async_callback_type& completion() { return _callback; }

        /**
         * @brief skips evaluation of Idx'th facet offering feature FeatureX because it doesn't exist, then passes to the next feature
         */
        template <std::size_t Idx, typename... Args, std::enable_if_t<std::is_void_v<typename fabric_type::template facet_type<FeatureX, Idx>>, bool> = true>
        void eval(Args&&... args){
            std::cout << "evaluation_helper<" << Stage << "," << FeatureX::name << (std::string("") + ... + ("," + std::string(Features::name))) << ">";
            std::cout << "::handler<JournalT, fabric<" << udho::manifold::facets_name<Facets...>::get() << ">";
            std::cout << udho::utils::format("::eval()<{}>(...)", Idx) << " [rest]" << std::endl;

            using args_tuple_type   = std::tuple<Args&&...>;
            using rest_handler_type = typename evaluator_helper<Stage, Features...>::template handler<journal_type, Facets...>;

            rest_handler_type rest_handler{_fabric, _journal, _callback};
            rest_handler.template operator()<0, Args...>(std::forward<Args>(args)...);
        }

        /**
         * @brief evaluates the Idx'th facet offering feature FeatureX, while passing the next_evaluator_helper pointing at the (Idx+1)'th facet offering the same feature
         * @pre fabric must contain atleast Idx+1 number of facets offering feature FeatureX for this function to be enabled
         */
        template <std::size_t Idx, typename... Args, std::enable_if_t<(fabric_type::template count<FeatureX>() > 0 && fabric_type::template count<FeatureX>()-1 > Idx), bool> = true>
        void eval(Args&&... args){
            std::cout << "evaluation_helper<" << Stage << "," << FeatureX::name << (std::string("") + ... + ("," + std::string(Features::name))) << ">";
            std::cout << "::handler<JournalT, fabric<" << udho::manifold::facets_name<Facets...>::get() << ">";
            std::cout << udho::utils::format("::eval()<{}>(...)", Idx) << " [next]" << std::endl;

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
            std::cout << "evaluation_helper<" << Stage << "," << FeatureX::name << (std::string("") + ... + ("," + std::string(Features::name))) << ">";
            std::cout << "::handler<JournalT, fabric<" << udho::manifold::facets_name<Facets...>::get() << ">";
            std::cout << udho::utils::format("::eval()<{}>(...)", Idx) << " [last]" << std::endl;

            using facet_type        = typename fabric_type::template facet_type<FeatureX, Idx>;
            using args_tuple_type   = std::tuple<Args&&...>;
            using rest_handler_type = typename evaluator_helper<Stage, Features...>::template handler<journal_type, Facets...>;
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
            std::cout << "evaluation_helper<" << Stage << "," << FeatureX::name << (std::string("") + ... + ("," + std::string(Features::name))) << ">";
            std::cout << "::handler<JournalT, fabric<" << udho::manifold::facets_name<Facets...>::get() << ">";
            std::cout << udho::utils::format("::operator()<{}>(...)", Idx) << std::endl;
            eval<Idx>(std::forward<Args>(args)...);
        }

        private:
            fabric_type&         _fabric;
            journal_type&        _journal;
            async_callback_type& _callback;
    };


};

/**
 * @brief The evaluator_helper specialization with no feature, intended to be called in the end of evaluating all features unless one intermediate facet fails
 */
template <std::size_t Stage>
struct evaluator_helper<Stage>{
    template <typename JournalT, typename... Facets>
    struct handler{
        using handler_type = handler<JournalT, Facets...>;
        using fabric_type  = udho::manifold::fabric<Stage, Facets...>;
        using journal_type = JournalT; // typename udho::manifold::detail::journal_for_facets<Facets...>::type;
        using safe_success_type = exclusive_result;
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
            fabric_type&         _fabric;
            journal_type&        _journal;
            async_callback_type& _callback;
    };
};

}

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_EVALUATOR_HELPER_H
