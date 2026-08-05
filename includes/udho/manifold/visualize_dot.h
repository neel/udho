#ifndef UDHO_MANIFOLD_VISUALIZE_H
#define UDHO_MANIFOLD_VISUALIZE_H

#include <udho/utils/format.h>
#include <udho/manifold/composition.h>

namespace udho{
namespace manifold{

/**
 * @ingroup DoxyG_manifold
 * @{
 */

namespace vis{
namespace dot{

std::string& dot_escape(std::string& label, bool escape = true) {
    if(!escape) return label;
    boost::replace_all(label, "<", "\\<");
    boost::replace_all(label, ">", "\\>");
    boost::replace_all(label, " ", "");
    return label;
}

template <typename Component>
std::string component_name(bool escape = true) {
    std::string name  = std::string(udho::manifold::component_name<Component>());
    return escape ? dot_escape(name) : name;
}
template <typename Feature>
std::string feature_name(bool escape = true) {
    std::string name = udho::utils::format("{} {}", Feature::stage, Feature::name);
    return escape ? dot_escape(name) : name;
}

template <typename Facet>
std::string facet_name(bool escape = true) {
    std::string name = std::string(udho::manifold::facet_name<Facet>::get());
    return escape ? dot_escape(name) : name;
}

template <typename... Features>
std::string features_concat(udho::manifold::features<Features...>, char delim = '|', bool escape = true){
    std::string concated = ((feature_name<Features>(escape) + delim) + ...);
    concated.pop_back();
    return concated;
}

template <typename Component>
std::string component_node(bool escape = true) {
    std::string comp_name  = component_name<Component>(escape);
    return udho::utils::format(
        "\"node_component_{}\" [\n"
            "\tlabel=\"{}\";\n"
        "];",
        comp_name, comp_name
    );
}
template <typename Facet>
std::string facet_node(bool escape = true) {
    std::string fac_name  = facet_name<Facet>(escape);
    return udho::utils::format(
        "\"node_facet_{}\" [\n"
            "\tlabel=\"{}\";\n"
        "];",
        fac_name, fac_name
    );
}

template <typename... Components>
std::string components_nodes(bool escape = true) {
    return (std::string(component_node<Components>(escape)) + ...);
}

template <typename... Facets>
std::string facets_nodes(bool escape = true) {
    return (std::string(facet_node<Facets>(escape)) + ...);
}

template <typename... Components>
std::string components_sequence(bool escape = true) {
    return ((std::string("\"node_component_" + component_name<Components>(escape)) +  "\"" + ";") + ...);
}

template <typename... Facets>
std::string facets_sequence(bool escape = true) {
    return ((std::string("\"node_facet_" + facet_name<Facets>(escape)) + "\"" + ";") + ...);
}

}
}

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

template <>
std::string visualize_features_dot(udho::manifold::features<>){
    return "";
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

template <typename Component, typename Feature>
std::string visualize_facet_dot(const udho::manifold::facet<Component, Feature>&){
    std::string facet_name  = std::string(udho::manifold::facet_name<udho::manifold::facet<Component, Feature>>::get());

    boost::replace_all(facet_name, "<", "\\<");
    boost::replace_all(facet_name, ">", "\\>");
    boost::replace_all(facet_name, " ", "");

    return udho::utils::format(
        "\"{}\" [\n"
            "\tlabel=\"{}\";\n"
        "];",
        facet_name, facet_name
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

    // std::string components_string = ((visualize_component_dot(composition.template get<Components>()) + "\n") + ...);

    // std::string components_semicolon = ((std::string("component_") + std::string(udho::manifold::component_name<Components>()) + ";") + ...);

    std::string components_nodes = vis::dot::components_nodes<Components...>(true);
    std::string components_edges = vis::dot::components_sequence<Components...>(true);

    // ordering chain: component_A -> component_B -> component_C;
    // std::string components_arrows = ((std::string("component_") + std::string(udho::manifold::component_name<Components>()) + "->") + ...);
    // components_arrows.pop_back();
    // components_arrows.pop_back();
    // components_arrows.push_back(';');

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
    stream << components_nodes << "\n";
    stream << "    {rank=same; " << components_edges << "}\n";
    // stream << "    " << components_arrows << "\n";
    stream << "  }\n";   // end cluster_composition
    stream << "}\n";     // end digraph

    return stream;
}

template <std::size_t Stage, typename... Facets>
std::ostream& visualize_fabric_dot(const udho::manifold::fabric<Stage, Facets...>& fabric, std::ostream& stream){
    stream << R"(digraph FabricRibbon {
                    graph [
                        rankdir=TB,
                        nodesep=0.65,
                        ranksep=0.80,
                        fontname="Helvetica",
                        bgcolor="white",
                        pad=0.20
                    ];

                    edge [style=invis, weight=50, minlen=2];

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

    std::string facets_nodes = vis::dot::facets_nodes<Facets...>(true);
    std::string facets_edges = vis::dot::facets_sequence<Facets...>(true);

    std::string facet_header = udho::utils::format(R"(
      subgraph cluster_composition {{
        label="fabric<{}, ...>";
        labelloc="t";
        labeljust="c";
        style="filled,rounded";
        fillcolor="#F4F7FC";
        color="#CBD5E1";
        penwidth=1.0;
    )", Stage);

    stream << facet_header << "\n";
    stream << facets_nodes << "\n";
    stream << "    {rank=same; " << facets_edges << "}\n";
    // stream << "    " << facets_arrows << "\n";
    stream << "  }\n";   // end cluster_composition
    stream << "}\n";     // end digraph

    return stream;
}

/**
 * @}
 */

}
}

#endif // UDHO_MANIFOLD_VISUALIZE_H
