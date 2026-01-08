#ifndef UDHO_MANIFOLD_VISUALIZE_H
#define UDHO_MANIFOLD_VISUALIZE_H

#include <udho/utils/format.h>
#include <udho/manifold/composition.h>

namespace udho{
namespace manifold{


template <typename Feature>
std::string visualize_feature_dot(){
    return udho::utils::format("{} {}", Feature::stage, Feature::name);
};

template <typename... Features>
std::string visualize_features_dot(udho::manifold::features<Features...>){
    std::string concated = ((visualize_feature_dot<Features>() + "|") + ...);
    concated.pop_back();
    return concated;
}

template <typename Component>
std::string visualize_component_dot(const udho::manifold::wrapper<Component>&){
    using features = typename Component::features;

    std::string component_name  = std::string(udho::manifold::component_name<Component>());
    std::string features_string = visualize_features_dot(features{});

    // record label: {ComponentName|{feature1|feature2|...}}
    std::string label = "{" + component_name + "|{" + features_string + "}}";

    return udho::utils::format(
            "component_{} [\n"
                "\tlabel=\"{}\";\n"
            "];",
            component_name, label
        );
}

template <typename... Components>
std::ostream& visualize_composition_dot(const udho::manifold::composition<Components...>& composition, std::ostream& stream){
    stream << R"(digraph CompositionRibbon {
                    graph [
                        rankdir=TB,
                        nodesep=0.65,
                        ranksep=0.80,
                        fontname="Helvetica",
                        bgcolor="white",
                        pad=0.20
                    ];

                    // Use invisible edges for ordering only
                    edge [style=invis, weight=50, minlen=2];

                    // Component nodes (record boxes)
                    node [
                        fontname="Helvetica",
                        shape=record,
                        style="rounded,filled",
                        fillcolor="#E6EEF9",
                        color="#A7B6CC",
                        penwidth=1.0,
                        margin="0.12,0.10"
                    ];
            )";

    std::string components_string = ((visualize_component_dot(composition.template get<Components>()) + "\n") + ...);

    std::string components_semicolon = ((std::string("component_") + std::string(udho::manifold::component_name<Components>()) + ";") + ...);

    // ordering chain: component_A -> component_B -> component_C;
    std::string components_arrows = ((std::string("component_") + std::string(udho::manifold::component_name<Components>()) + "->") + ...);
    components_arrows.pop_back();
    components_arrows.pop_back();
    components_arrows.push_back(';');

    std::string composition_header = R"(
      subgraph cluster_composition {
        label="composition";
        labelloc="t";
        labeljust="c";
        style="filled,rounded";
        fillcolor="#F4F7FC";
        color="#CBD5E1";
        penwidth=1.0;
    )";

    stream << composition_header << "\n";
    stream << components_string << "\n";
    stream << "    {rank=same; " << components_semicolon << "}\n";
    // stream << "    " << components_arrows << "\n";
    stream << "  }\n";   // end cluster_composition
    stream << "}\n";     // end digraph

    return stream;
}


}
}

#endif // UDHO_MANIFOLD_VISUALIZE_H
