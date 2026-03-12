#ifndef UDHP_MANIFOLD_VISUALIZE_HTML_H
#define UDHP_MANIFOLD_VISUALIZE_HTML_H

#include <udho/utils/format.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/pipeline.h>
#include <udho/utils/encoding.h>
#include <udho/www/components/protocol.h>
#include <boost/lexical_cast.hpp>

namespace udho{
namespace manifold{


namespace vis{
namespace html{

template <typename Component>
std::string component_name() {
    std::string name  = std::string(udho::manifold::component_name<Component>());
    return udho::utils::encode::escape(name);
}

template <typename Feature>
std::string feature_stage() {
    return udho::utils::format("{}", Feature::stage);
}

template <typename Feature>
std::string feature_name() {
    return udho::utils::encode::escape(std::string(Feature::name));
}

template <typename Facet>
std::string facet_name() {
    std::string name = std::string(udho::manifold::facet_name<Facet>::get());
    return udho::utils::encode::escape(name);
}

struct config_html_serializer{
    inline config_html_serializer(std::stringstream& stream): _stream(stream) {}

    template <typename ParamT>
    void operator()(const ParamT& d){
        _stream << row(d);
    }

    template <typename ParamT>
    std::string row(const ParamT& d) {
        return udho::utils::format(
            "<tr> <td class='t'>{}</td> <td class='k'>{}</td> <td class='v'>{}</td></tr>",
            udho::utils::encode::escape(_type(d.value())),
            udho::utils::encode::escape(d.name()),
            _value(d.value())
        );
    }

    template <typename Value, std::enable_if_t<udho::utils::traits::is_ostreamable_v<Value>, bool> = true>
    std::string _value(const Value& v) const {
        return boost::lexical_cast<std::string>(v);
    }

    std::string _type(bool) const { return "bool"; }

    std::string _type(char) const { return "char"; }
    std::string _type(signed char) const { return "int8"; }
    std::string _type(unsigned char) const { return "uint8"; }

#if __cpp_char8_t
    std::string _type(char8_t) const { return "char8_t"; }
#endif
    std::string _type(char16_t) const { return "char16_t"; }
    std::string _type(char32_t) const { return "char32_t"; }
    std::string _type(wchar_t) const { return "wchar_t"; }

    // fixed-width integers
    std::string _type(std::int16_t) const { return "int16"; }
    std::string _type(std::uint16_t) const { return "uint16"; }
    std::string _type(std::int32_t) const { return "int32"; }
    std::string _type(std::uint32_t) const { return "uint32"; }
    std::string _type(std::int64_t) const { return "int64"; }
    std::string _type(std::uint64_t) const { return "uint64"; }

    // floats
    std::string _type(float) const { return "float"; }
    std::string _type(double) const { return "double"; }
    std::string _type(long double) const { return "long double"; }

    // strings
    std::string _type(const std::string&) const { return "string"; }
    std::string _type(std::string_view) const { return "string_view"; }

    // fallback
    template <typename T>
    std::string _type(const T&) const { return "Type"; }

    std::stringstream& _stream;
};

template <typename Feature>
struct feature_block{
    static std::string apply() {
        return udho::utils::format(
            R"(
            <div class="feature">
                <span class="s">{}</span>
                <span class="n">{}</span>
            </div>
            )",
            feature_stage<Feature>(),
            feature_name<Feature>()
        );
    }
};

template <typename Features>
struct features_block;

template <typename... Features>
struct features_block<udho::manifold::features<Features...>>{
    static std::string apply() {
        return udho::utils::format(
            R"(<div class="features"> {} </div>)",
            (std::string{} + ... + feature_block<Features>::apply())
        );
    }
};

template <typename ConfigT>
struct params_block;

template <typename Component>
struct params_block<udho::manifold::config<Component>>{
    static std::string apply(const udho::manifold::config<Component>& config) {
        std::stringstream stream;
        stream << "<table>";
        config.apply(config_html_serializer(stream));
        stream << "</table>";
        return stream.str();
    }
};


template <typename Component>
struct config_block{
    static std::string apply(const udho::manifold::config<Component>& config) {
        std::string params_str = params_block<udho::manifold::config<Component>>::apply(config);
        return "<div class='node config'><div class='head'>" + component_name<Component>() + "</div><div class='params'>" + params_str + "</div></div>";
    }
};

template <typename Component>
struct component_block{
    static std::string apply(const Component& c, const udho::manifold::config<Component>& cnf) {
        std::string table = params_block<udho::manifold::config<Component>>::apply(cnf);

        return udho::utils::format(
            R"(
                <div class="node component">
                    <div class="head">{}</div>
                    {}
                    <div class="params">
                        {}
                    </div>
                </div>
            )",
            component_name<Component>(),
            features_block<typename Component::features>::apply(),
            table
        );
    }
};

template <typename Facet>
struct facet_block;

template <typename Component, typename Feature>
struct facet_block<udho::manifold::facet<Component, Feature>>{
    static std::string apply() {
        return udho::utils::format(
            R"(
                <div class="node facet">
                    <div class="head">{}</div>
                    <div class="features"> {} </div>
                    <div class="result {}">
                        <div class="head">{}</div>
                    </div>
                </div>
            )",
            component_name<Component>(),
            feature_block<Feature>::apply(),
            (udho::manifold::has_result<udho::manifold::facet<Component, Feature>>::value ? std::string("") : std::string("empty")),
            (udho::manifold::has_result<udho::manifold::facet<Component, Feature>>::value ? feature_name<Feature>()+"::result" : std::string(""))
        );
    }
};

template <typename Composition>
struct composition_block;

template <typename... Components>
struct composition_block<udho::manifold::composition<Components...>>{
    static std::string apply(const udho::manifold::composition<Components...>& comp, const udho::manifold::configs<Components...>& cnfs) {
        return udho::utils::format(
            R"(
                <section class="composition">
                    <div class="head">composition</div>
                    <div class="row components">
                        {}
                    </div>
                </section>
            )",
            (std::string{} + ... + component_block<Components>::apply(comp.template get<Components>().component(), cnfs.template get<Components>()))
        );
    }
};

template <typename Fabric>
struct configs_block;

template <typename... Components>
struct configs_block<udho::manifold::configs<Components...>>{
    static std::string apply(const udho::manifold::configs<Components...>& configs) {
        return udho::utils::format(
            R"(
                <section class="configuration">
                    <div class="head">configs</div>
                    <div class="row configs">
                        {}
                    </div>
                </section>
            )",
            (std::string{} + ... + config_block<Components>::apply(configs.template get<Components>()))
        );
    }
};

template <typename Fabric>
struct fabric_block;

template <std::size_t Stage, typename... Facets>
struct fabric_block<udho::manifold::fabric<Stage, Facets...>>{
    static std::string apply() {
        return udho::utils::format(
            R"(
                <section class="pipeline">
                    <div class="head">pipeline&lt;{}&gt;</div>
                    <section class="fabric">
                        <div class="head">fabric&lt;{}&gt;</div>
                        <div class="row facets">
                            {}
                        </div>
                    </section>
                </section>
            )",
            Stage,
            Stage,
            (std::string{} + ... + facet_block<Facets>::apply())
        );
    }
};

template <typename Pipeline>
struct pipeline_block;

template <typename CompositionT, typename OrderT, std::size_t Count, int Stage>
struct pipeline_block<udho::manifold::pipeline<CompositionT, OrderT, Count, Stage>>{
    using pipeline_type         = udho::manifold::pipeline<CompositionT, OrderT, Count, Stage>;
    using common_pipeline_type  = typename pipeline_type::common_pipeline_type;
    using basic_pipeline_type   = typename common_pipeline_type::basic_pipeline_type;
    using fabric_type           = typename basic_pipeline_type::fabric_type;
    using next_pipeline_type    = udho::manifold::pipeline<CompositionT, OrderT, Count, Stage +1>;

    static std::string fabrics() {
        static std::string transition = R"(
            <section class="transition">
                <div class="head"> {} ⇒ {} </div>
            </section>
        )";

        std::string transition_str = (Stage+1 < Count) ? udho::utils::format(transition, Stage, Stage+1) : "";

        return fabric_block<fabric_type>::apply() + transition_str + pipeline_block<next_pipeline_type>::fabrics();
    }
};

template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline_block<udho::manifold::pipeline<CompositionT, OrderT, Count, static_cast<int>(Count)>>{
    static std::string fabrics() {
        return udho::utils::format(
            R"(
                <section class="pipeline">
                    <div class="head">finish</div>
                </section>
            )"
        );
    }
};

template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline_block<udho::manifold::pipeline<CompositionT, OrderT, Count, -1>>{
    using root_pipeline_type    = udho::manifold::pipeline<CompositionT, OrderT, Count, 0>;

    static std::string apply() {
        return udho::utils::format(
            R"(
                <section class="flow">
                    <div class="track"><div class="head">journal</div></div>
                    <div class="head">flow</div>
                    <section class="configuration">
                        <div class="head">configs</div>
                        <div class="row configs">
                        </div>
                    </section>
                    <div class="row pipelines">
                        <section class="pipeline">
                            <div class="head">start</div>
                        </section>
                        {}
                    </div>
                </section>
            )",
            pipeline_block<root_pipeline_type>::fabrics()
        );
    }

    static std::string apply(const typename CompositionT::configs_type& configs) {
        return udho::utils::format(
            R"(
                <section class="flow">
                    <div class="track"><div class="head">journal</div></div>
                    <div class="head">flow</div>
                    {}
                    <div class="row pipelines">
                        <section class="pipeline">
                            <div class="head">start</div>
                        </section>
                        {}
                    </div>
                </section>
            )",
            configs_block<typename CompositionT::configs_type>::apply(configs),
            pipeline_block<root_pipeline_type>::fabrics()
        );
    }
};

template <typename... Components>
static std::string composition(const udho::manifold::composition<Components...>& c) {
    return composition_block<udho::manifold::composition<Components...>>::apply(c);
}

template <typename CompositionT, typename OrderT, std::size_t Count>
static std::string pipeline(const udho::manifold::pipeline<CompositionT, OrderT, Count, -1>&) {
    return pipeline_block<udho::manifold::pipeline<CompositionT, OrderT, Count, -1>>::apply();
}

udho::utils::string_view css() {
   return R"CSS(
        :root{
          --bg: #ffffff;

          --panel-bg: #F4F7FC;
          --panel-border: #CBD5E1;

          --node-bg: #E6EEF9;
          --node-border: #A7B6CC;

          --text: #0f172a;
          --muted: #475569;

          --chip-bg: rgba(255,255,255,.70);
          --chip-border: rgba(15,23,42,.18);

          --gap: 14px;
          --radius: 12px;
        }

        * { box-sizing: border-box; }

        body{
            margin: 0;
            padding: 24px;
            background: var(--bg);
            color: var(--text);
            font: 14px/1.35 system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif;
        }

        .diagram{
            max-width: 1600px;
            margin: 0 auto;
            display: flex;
            flex-direction: column;
            gap: 18px;
            position: relative;
            font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
        }

        .composition, .pipeline, .configuration, .flow{
            border: 2px solid var(--panel-border);
            background: var(--panel-bg);
            border-radius: var(--radius);
            padding: 14px;
        }

        .flow .track {
            position: absolute;
            bottom: 78px;
            left: 14px;
            right: 0px;
            height: 24px;
            background: white;
            z-index: 1;
            margin: 0;
            padding: 2px 0px 0px 8px;
            border: 1px solid var(--panel-border);
            box-shadow: 0 1px 0 rgba(15, 23, 42, .06);
        }

        .head{
            font-weight: 650;
            color: var(--muted);
            margin: 0 0 10px 2px;
            letter-spacing: .2px;
        }

        .row.pipelines{
            gap: 18px;
            align-items: stretch;
            position: relative;
            padding-top: 18px;
        }

        .row.pipelines > .pipeline{
            flex: 0 0 auto;
        }

        .pipeline .fabric{
            min-width: unset;
            flex: 0 0 auto;
            border: 2px solid var(--panel-border);
            background: rgba(255,255,255,.55);
            border-radius: var(--radius);
            padding: 12px;
            display: flex;
            flex-direction: column;
            gap: 10px;
            position: relative;
        }

        .transition{
            flex: 0 0 auto;
            display: flex;
            align-items: center;
            justify-content: center;
            text-align: center;
        }

        .row{
            display: flex;
            flex-wrap: nowrap;
            gap: var(--gap);
            align-items: stretch;
            overflow-x: auto;
            padding-bottom: 6px;
        }

        .row::-webkit-scrollbar { height: 10px; }
        .row::-webkit-scrollbar-thumb { background: #cbd5e1; border-radius: 999px; }
        .row::-webkit-scrollbar-track { background: transparent; }

        .row.fabrics{
            gap: 18px;
        }

        .transition {
            border: 2px solid var(--panel-border);
            background: rgba(255, 255, 255, .55);
            border-radius: var(--radius);
            padding: 12px;
        }

        .node{
            display: grid;
            grid-template-columns: 1fr auto;
            grid-template-rows: auto;
            column-gap: 12px;
            align-items: start;
            background: var(--node-bg);
            border: 1.5px solid var(--node-border);
            border-radius: 10px;
            padding: 10px 10px 9px;
            grid-template-rows: auto 1fr auto;
        }

        .features{
            grid-column: 2;
            justify-self: end;
            display: flex;
            flex-direction: column;
            gap: 2px;
            align-items: stretch;
            flex-wrap: nowrap;
        }

        .feature{
            display: flex;
            width: auto;
            font-size: 12px;
            padding: 8px 8px;
            border-radius: 999px;
            background: var(--chip-bg);
            border: 1px solid var(--chip-border);
            color: #0b1220;
            padding-top: 3px;
            padding-bottom: 3px;
        }

        .feature .s{
            font-weight: 700;
            border-radius: 7px;
            margin-right: 4px;
            padding: 0px 4px 0px 4px;
            background: #1f77b4;
            color: white;
        }
        .feature .n{
            color: #0b1220;
        }

        .node.component{
            border-left-width: 4px;
            border-left-color: #1f77b4;
        }
        .node.facet{
            border-left-width: 4px;
            border-left-color: #ff7f0e;
            background: rgba(230,238,249,.75);
            padding-top: 18px;
            position: relative;
            min-height: 90px;
            z-index: 1;
        }

        .node.facet .feature .s{
            background: #ff7f0e;
        }

        .node.facet .result{
            position: absolute;
            background: white;
            font-size: 12px;
            font-weight: 700;
            color: var(--muted);
            z-index: 2;
            height: 24px;
            bottom: 9px;
            border: 1px solid var(--panel-border);
            left: -4px;
            right: -1px;
            border-left: 0;
            border-right: 0;
        }

        .result.empty{
            background: transparent !important;
            border: 0 !important;
        }

        .row.facets{
            position: relative;
            padding-top: 18px;
        }

        .node.facet .result > .head{
            margin: 0;
            font-size: inherit;
            font-weight: inherit;
            color: inherit;
            margin-top: 3px;
            font-weight: normal;
        }

        .node.facet .result:not(.empty) > .head::before{
            content: "⛁";
            font-size: 12px;
            line-height: 1;
            color: var(--muted);
            opacity: 0.9;
            margin-right: 5px;
            color: #ff7f0e;
        }

        .result.empty > .head{
            opacity: 0.75;
        }

        .params{
            grid-column: 1 / -1;
            grid-row: 3;
        }

        .params > table{
            table-layout: fixed;
            border-collapse: collapse;
            margin-top: 6px;
            font-size: 12px;
        }

        .params td{
            padding: 4px 6px;
            border-top: 1px solid rgba(15,23,42,.12);
            vertical-align: top;
        }
        .params tr:first-child td{ border-top: 0; }

        .params td.t{
            white-space: nowrap;
            opacity: 0.85;
        }
        .params td.k{
            white-space: nowrap;
            font-weight: 650;
        }
        .params td.v{
            width: auto;
            word-break: break-word;
        }

        .listener {
            position: absolute;
            left: -50px;
            top: 0px;
            bottom: 0px;
            width: 34px;
            border: 2px solid var(--panel-border);
            background: white;
            border-radius: 10px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-weight: 700;
            color: var(--muted);
            letter-spacing: .3px;
            writing-mode: vertical-rl;
            text-orientation: mixed;
            transform: rotate(180deg);
        }
    )CSS";
}

template <typename LabelT, typename StreamT>
static std::string runtime(const udho::manifold::basic_runtime<LabelT, StreamT>& runtime) {
    using runtime_type          = udho::manifold::basic_runtime<LabelT, StreamT>;
    using start_pipeline_type   = typename runtime_type::start_pipeline_type;
    using composition_type      = typename start_pipeline_type::composition_type;

    return composition_block<composition_type>::apply(runtime.composition(), runtime.baseline());
    // std::string pipeline_str    = pipeline_block<start_pipeline_type>::apply();
}

template <typename LabelT, typename StreamT>
static std::ostream& runtime(std::ostream& stream, const udho::manifold::basic_runtime<LabelT, StreamT>& r) {
    using start_pipeline_type   = typename udho::manifold::basic_runtime<LabelT, StreamT>::start_pipeline_type;
    // std::string pipeline_str    = pipeline_block<start_pipeline_type>::apply(r.baseline());
    std::string pipeline_str    = pipeline_block<start_pipeline_type>::apply();
    stream << udho::utils::format(
        R"(<!doctype html>
            <html lang="en">
            <head>
              <meta charset="utf-8" />
              <meta name="viewport" content="width=device-width,initial-scale=1" />
              <title>manifold pipeline structure</title>
              <style>{}</style>
            </head>
            <body>
                <div class="diagram">
                <div class="listener">listener</div>
                {}
                {}
                </div>
            </body>
            </html>
        )",
        css(),
        runtime(r),
        pipeline_str
    );
    return stream;
}

template <typename LabelT, typename StreamT>
static std::ostream& flow(std::ostream& stream, const udho::manifold::basic_flow<LabelT, StreamT>& f) {
    using start_pipeline_type   = typename udho::manifold::basic_flow<LabelT, StreamT>::start_pipeline_type;

    std::string pipeline_str    = pipeline_block<start_pipeline_type>::apply(f.root());

    stream << udho::utils::format(
        R"(<!doctype html>
            <html lang="en">
            <head>
              <meta charset="utf-8" />
              <meta name="viewport" content="width=device-width,initial-scale=1" />
              <title>manifold pipeline structure</title>
              <style>{}</style>
            </head>
            <body>
                <div class="diagram">
                <div class="listener">listener</div>
                {}
                {}
                </div>
            </body>
            </html>
        )",
        css(),
        runtime(f.runtime()),
        pipeline_str
    );
    return stream;
}

}
}

}
}

#endif // UDHP_MANIFOLD_VISUALIZE_HTML_H
