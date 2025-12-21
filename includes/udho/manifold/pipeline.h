#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/evaluator.h>

namespace udho {
namespace manifold {

namespace detail{

template <typename... Components>
struct get_journal_for_all_components{
    using full_fabric_type = typename udho::manifold::detail::flatten_all_of<Components...>::type;
    using type = typename udho::manifold::detail::journal_for_fabric<full_fabric_type>::type;
};

template <typename CompositionT>
struct get_journal_for_full_fabric;

template <typename... Components>
struct get_journal_for_full_fabric<udho::manifold::composition<Components...>>: get_journal_for_all_components<Components...>{};


template <typename JournalT, typename FabricT>
struct get_handler_type;

template <typename JournalT, std::size_t Stage, typename... Facets>
struct get_handler_type<JournalT, udho::manifold::fabric<Stage, Facets...>>{
    template <typename... Features>
    using for_features = typename detail::evaluator_helper<Stage, Features...>::template handler<JournalT, Facets...>;
};

};

template <std::size_t Stage, typename... Components>
struct basic_pipeline{
    using fabric_type       = typename udho::manifold::detail::flatten_all<Stage, Components...>::type;
    // using journal_type     = typename udho::manifold::detail::journal_for_fabric<fabric_type>::type;
    using full_journal_type = typename detail::get_journal_for_all_components<Components...>::type;

    template <typename... XComponents>
    basic_pipeline(udho::manifold::composition<XComponents...>& composition, const udho::manifold::configs<XComponents...>& configs): _fabric(composition, configs) {}

    template <typename... Features>
    struct evaluator{
        using helper_type         = detail::evaluator_helper<Stage, Features...>;
        using handler_type        = typename detail::get_handler_type<full_journal_type, fabric_type>::template for_features<Features...>;
        using safe_success_type   = typename handler_type::safe_success_type;
        using async_callback_type = typename handler_type::async_callback_type;

        evaluator(fabric_type& fabric, full_journal_type& journal, async_callback_type& callback): _handler(fabric, journal, callback) {}

        template <typename... Args>
        void eval(Args&&... args){
            _handler.template operator()<0>(std::forward<Args>(args)...);
        }

        private:
        handler_type _handler;
    };

    fabric_type& fabric() { return _fabric; }
    const fabric_type& fabric() const { return _fabric; }

    // journal_type& journal() { return _journal; }
    // const journal_type& journal() const { return _journal; }

    private:
    fabric_type  _fabric;
    // journal_type _journal;

};

template <typename... Features>
struct order{};

template <std::size_t Stage, typename OrderT, typename CompositionT>
class common_pipepine;

template <std::size_t Stage, typename... Features, typename... Components>
class common_pipepine<Stage, order<Features...>, udho::manifold::composition<Components...>>: private basic_pipeline<Stage, Components...> {
    using composition_type    = udho::manifold::composition<Components...>;
    using basic_pipeline_type = basic_pipeline<Stage, Components...>;
    using evaluator_type      = typename basic_pipeline_type::template evaluator<Features...>;
    using safe_success_type   = typename evaluator_type::safe_success_type;
    using async_callback_type = typename evaluator_type::async_callback_type;

public:
    using configs_type        = udho::manifold::configs<Components...>;
    // using journal_type        = typename basic_pipeline_type::journal_type;
    using full_journal_type   = typename basic_pipeline_type::full_journal_type;

public:
    common_pipepine(composition_type& composition, const configs_type& configs, full_journal_type& journal):
        basic_pipeline_type(composition, configs),
        _evaluator(basic_pipeline_type::fabric(), journal, _callback),
        _callback(std::bind(&common_pipepine::on_completion, this, std::placeholders::_1))
    {}

    using basic_pipeline_type::fabric;
    // using basic_pipeline_type::journal;

    template <typename... Args>
    void eval(Args&&... args){
        _evaluator.eval(std::forward<Args>(args)...);
    }

    common_pipepine& then(async_callback_type&& callback){
        _user_callback = std::move(callback);
        return *this;
    }

    common_pipepine& then(boost::asio::io_context& io, async_callback_type&& callback){
        _user_callback = [&io, &callback](safe_success_type&& success){
            boost::asio::post(io, std::bind(std::forward<async_callback_type>(callback), std::move(success)));
        };
        return *this;
    }
private:
    void on_completion(safe_success_type&& success){
        if(_user_callback) {
            // Will be called in case of failure
            _user_callback(std::forward<safe_success_type>(success));
        }
    }

private:
    async_callback_type _callback;
    async_callback_type _user_callback;
    evaluator_type      _evaluator;
};

template <typename LabelT>
struct flow;

template <typename CompositionT, typename OrderT, std::size_t Count, int Stage = 0>
struct pipeline{
    using composition_type   = CompositionT;
    using order_type         = OrderT;
    using pipeline_type      = udho::manifold::common_pipepine<Stage, order_type, composition_type>;
    using configs_type       = typename pipeline_type::configs_type;
    using journal_type       = typename pipeline_type::journal_type;
    using prev_pipeline_type = pipeline<composition_type, order_type, Count, Stage-1>;
    using next_pipeline_type = pipeline<composition_type, order_type, Count, Stage+1>;

    friend class pipeline<composition_type, order_type, Count, Stage-1>;
    // pipeline<..., Count, -1> would be maolformed, so making friend with pipeline<..., Count, Count> instead which is harmless

    template <typename JournalT>
    pipeline(composition_type& composition, typename composition_type::configs_type& baseline, JournalT& journal, const prev_pipeline_type& previous)
        : _composition(composition), _configs(baseline), _pipeline(composition, _configs), _next(composition, baseline, journal, *this), _previous(previous)
    {
        // _configs copy constructor picks the relevant configs from the baseline
    }

    const configs_type& configs() const { return _configs; }

    const journal_type& journal() const { return _pipeline.journal(); }

    template <typename FlowT>
    void operator()(std::shared_ptr<FlowT> flow){
        next_pipeline_type& next    = _next;
        const journal_type& journal = _pipeline.journal();

        _pipeline.then([flow, &next, this](std::variant<bool, std::exception_ptr> success){
            if(success.index() == 0 && std::get<0>(success)) {
                typename next_pipeline_type::configs_type& next_configs = next.configs();
                flow->apply(_pipeline, next_configs);
                next(flow);
            } else {
                // terminate
            }
        }).eval();
    }

    template <typename FlowT>
    void operator()(boost::asio::io_context& io, std::shared_ptr<FlowT> flow){
        next_pipeline_type& next    = _next;
        const journal_type& journal = _pipeline.journal();

        _pipeline.then(io, [flow, &io, &next, this](std::variant<bool, std::exception_ptr> success){
            if(success.index() == 0 && std::get<0>(success)) {
                typename next_pipeline_type::configs_type& next_configs = next.configs();
                flow->apply(_pipeline, next_configs);
                next(io, flow);
            } else {
                // terminate
            }
        }).eval();
    }

    template <std::size_t N, std::enable_if_t<(N == Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return *this; }

    template <std::size_t N, std::enable_if_t<(N == Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return *this; }

    template <std::size_t N, std::enable_if_t<(N > Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return _next.template at<N>(); }

    template <std::size_t N, std::enable_if_t<(N > Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _next.template at<N>(); }

    template <std::size_t N, std::enable_if_t<(N < Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _previous.template at<N>(); }

private:
    configs_type& configs() { return _configs; }

private:
    composition_type&   _composition;
    const prev_pipeline_type& _previous;
    configs_type        _configs;   // per stage copy of configs
    pipeline_type       _pipeline;
    next_pipeline_type  _next;
};

template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline<CompositionT, OrderT, Count, Count>{
    using prev_pipeline_type = pipeline<CompositionT, OrderT, Count, Count-1>;
    using configs_type = udho::manifold::configs<>;

    template <typename JournalT>
    pipeline(CompositionT&, typename CompositionT::configs_type&, JournalT&, const prev_pipeline_type& previous): _previous(previous) {}

    configs_type& configs() { return _configs; }

    template <typename... Args>
    void operator()(Args&&...){
        // Finished
    }

private:
    configs_type _configs;
    const prev_pipeline_type& _previous;
};

template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline<CompositionT, OrderT, Count, -1> {
    using composition_type   = CompositionT;
    using order_type         = OrderT;
    using full_journal_type  = typename detail::get_journal_for_full_fabric<CompositionT>::type;
    using next_pipeline_type = pipeline<composition_type, order_type, Count, 0>;
    using self_type          = pipeline<CompositionT, OrderT, Count, -1>;
    using ptr                = std::shared_ptr<self_type>;

    pipeline(CompositionT& composition, typename CompositionT::configs_type& baseline): _composition(composition), _next(composition, baseline, _journal, *this) {}

    template <typename... Args>
    void operator()(Args&&... args){
        next(std::forward<Args>(args)...);
    }

    template <std::size_t N>
    pipeline<composition_type, order_type, Count, N>& at() { return _next.template at<N>(); }

    template <std::size_t N>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _next.template at<N>(); }

    const full_journal_type& journal() const { return _journal; }

private:
    composition_type&   _composition;
    full_journal_type   _journal;
    next_pipeline_type  _next;
};

/**
 * @brief provides the sketch of executaion plan for the label
 * The label can be any emoty struct
 * The sketch specialization must contain the following typedefs
 * - composition_type
 * - order_type
 * - Count
 *
 */
template <typename LabelT>
struct sketch;

template <typename LabelT>
struct runtime{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using flow_type         = flow<label_type>;
    using flow_ptr_type     = std::shared_ptr<flow_type>;
    using flow_wptr_type    = std::weak_ptr<flow_type>;
    using collection_type   = std::vector<flow_wptr_type>;

    runtime(runtime&&) = delete;

    runtime& operator=(runtime&&) = delete;

    runtime(composition_type&& composition): _composition(std::move(composition)) {}

    composition_type& composition() { return _composition; }

    const composition_type& composition() const { return _composition; }

    const configs_type& baseline() const {return _baseline; }

    flow_ptr_type spawn() {
        std::scoped_lock lock(_mutex);
        flow_ptr_type flow_ptr = flow_type::create(*this);
        _flows.push_back(flow_ptr);
        return flow_ptr;
    }

    std::size_t count() const {
        std::scoped_lock<std::mutex> lock(_mutex);
        return _flows.size();;
    }

    void cleanup() {
        // will be invoked through a timer externally
        std::scoped_lock<std::mutex> lock(_mutex);
        _flows.erase(std::remove_if(_flows.begin(), _flows.end(), [](flow_wptr_type& w){ return w.expired(); }), _flows.end());
    }

    // TODO load _baseline methods

private:
    composition_type _composition;
    configs_type     _baseline;
private:
    collection_type    _flows;
    mutable std::mutex _mutex;
};

template <typename LabelT, std::size_t Stage>
struct patch_config{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;

    static constexpr std::size_t Count = sketch_type::Count;

    using pipeline_type     = pipeline<composition_type, order_type, Count, Stage>;
    using next_config_type  = typename pipeline<composition_type, order_type, Count, Stage+1>::configs_type;

    void apply(const pipeline_type& p, next_config_type& config) { /* nothing unless specialized */ }
};

namespace detail {

template <typename LabelT, std::size_t Count, std::size_t Stage>
struct patcher: public detail::patcher<LabelT, Count, Stage+1>, private patch_config<LabelT, Stage>{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using pipeline_type     = pipeline<composition_type, order_type, Count, Stage>;
    using next_config_type  = typename pipeline<composition_type, order_type, Count, Stage+1>::configs_type;

    void apply(const pipeline_type& p, next_config_type& configs){
        patch_config<LabelT, Stage>::apply(p, configs);
    }
};

template <typename LabelT, std::size_t Count>
struct patcher<LabelT, Count, Count>{};

}

template <typename LabelT>
struct flow: public std::enable_shared_from_this<flow<LabelT>>, detail::patcher<LabelT, sketch<LabelT>::Count, 0>{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using engine_type       = runtime<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;

    static constexpr std::size_t Count = sketch_type::Count;

    using pipeline_type     = pipeline<composition_type, order_type, Count, -1>;
    using configs_type      = typename composition_type::configs_type;
    using ptr               = std::shared_ptr<flow<LabelT>>;

    template <typename>
    friend struct runtime;

    flow() = delete;
    flow(const flow<LabelT>&) = delete;
    flow(flow<LabelT>&&) = delete;

    ptr self() { return std::enable_shared_from_this<flow<LabelT>>::shared_from_this(); }

    template <std::size_t Stage>
    void apply(const pipeline<composition_type, order_type, Count, Stage>& p, typename pipeline<composition_type, order_type, Count, Stage+1>::configs_type& config){
        detail::patcher<LabelT, sketch<LabelT>::Count, 0>::apply(p, config);
    }

    void start() { _pipeline(self()); }

    void start(boost::asio::io_context& io) { _pipeline(io, self()); }

private:
    flow(composition_type& composition, const configs_type& baseline): _pipeline(composition, baseline) {}

    static ptr create(engine_type& engine) {
        return ptr(new flow(engine.composition(), engine.baseline()));
    }

private:
    pipeline_type _pipeline;

};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
