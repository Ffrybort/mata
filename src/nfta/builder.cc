#include "mata/nfta/builder.hh"

namespace mata::nfta {

Nfta create_universal(RankedAlphabet *alphabet) {
    Nfta aut({ 0 }, alphabet, Delta(1));
    bool has_constant = false;
    for (const auto [symbol, arity] : alphabet->get_alphabet_symbols_arities()) {
        const std::vector<State> targets(arity, 0);
        aut.delta.add(0, symbol, targets);
        if (arity == 0) { has_constant = true; }
    }

    if (!has_constant) {
        std::cerr << "warning: mata::nfta::create_universal alphabet does not contain any constant" << std::endl;
    }
    return aut;
} // create_universal

Nfta create_universal(const utils::OrdVector<SymbolArity> *symbols, Alphabet *alphabet) {
    Nfta aut({ 0 }, alphabet, Delta(1));
    bool has_constant = false;
    for (const auto [symbol, arity] : *symbols) {
        const std::vector<State> targets(arity, 0);
        aut.delta.add(0, symbol, targets);
        if (arity == 0) { has_constant = true; }
    }

    if (!has_constant) {
        std::cerr << "warning: mata::nfta::create_universal alphabet does not contain any constant" << std::endl;
    }
    return aut;
} // create_universal

/// brief Get state from a string, insert to @p delta if not already in the @p state_map.
static State get_state(const std::string& state_str, std::unordered_map<std::string, State>& state_map, Delta& delta) {
    auto [it, inserted] = state_map.try_emplace(state_str, State{});
    if (inserted) { it->second = delta.add_state(); }
    return it->second;
}

/// Add initial and final states as final states to nfta.
void add_initial_and_final_states(const IntermediateAut* inter_aut, NameStateMap &state_map, Nfta &aut) {
    auto add_states = [&](const auto& states) {
        for (const auto& state_str : states) {
            const State state = get_state(state_str, state_map, aut.delta);
            aut.add_initial_state(state);
        }
    };
    add_states(inter_aut->initial_formula.collect_node_names());
    add_states(inter_aut->final_formula.collect_node_names());
} // add_initial_and_final_states

/**
 * @brief Helper function to extract a transition from an inter_aut.
 * @param formula_node A node containing the left-hand side (source state).
 * @param formula_graph A graph containing the right-hand side (symbol and target(s)).
 * @param alphabet
 * @param state_map A mapping of state names to numbers used by the constructor.
 * @return A transition, with the symbol remaining a std::string, and states translated to internal numeric values.
 *
 * The reason for not translating a symbol right away is there is currently no unified way to do so for every possible
 * construction.
 */
RawTransition get_transition(
    const FormulaNode &formula_node, const FormulaGraph &formula_graph, const NameStateMap &state_map) {
    RawTransition transition;
    transition.source = state_map.at(formula_node.name);

    transition.symbol_string = formula_graph.node.is_and()
        ? formula_graph.children[0].node.name
        : formula_graph.node.name;

    if (std::size_t num_of_children = formula_graph.children.size();
            num_of_children == 2 && !formula_graph.children[1].node.is_and()) { // single target
        transition.targets.push_back(state_map.at(formula_graph.children[1].node.name));
    } else if (num_of_children == 2) {
        auto *tmp_graph = &formula_graph.children[1];
        num_of_children = tmp_graph->children.size();
        // walk through right side chain of & nodes
        while (num_of_children == 2 && tmp_graph->children[1].node.is_and()) {
            if(!tmp_graph->children[1].node.is_symbol()) { // skip the symbol
                transition.targets.push_back(state_map.at(tmp_graph->children[0].node.name));
            }
            tmp_graph = &tmp_graph->children[1];
            num_of_children = tmp_graph->children.size();
        }

        // final node - add both children
        if (num_of_children >= 1) {
            transition.targets.push_back(state_map.at(tmp_graph->children[0].node.name));
        }
        if (num_of_children == 2) {
            transition.targets.push_back(state_map.at(tmp_graph->children[1].node.name));
        }
    } // else if num_of_children == 2
    return transition;
} // get_transition

void add_states(NameStateMap& state_map, const std::vector<std::string>& state_names, Delta& delta) {
    for (const std::string& state_name : state_names) {
        state_map[state_name] = get_state(state_name, state_map, delta);
    }
} // add_states

Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, RankedAlphabet *alphabet) {
    // this might cause memory leaks if not handled right
    if (alphabet == nullptr) { alphabet = new RankedOnTheFlyAlphabet(); }
    Nfta aut;
    aut.alphabet = alphabet;
    NameStateMap state_map = {};
    add_states(state_map, inter_aut->states_names, aut.delta);

    // both initial and final states are added to initial_states in nfta
    add_initial_and_final_states(inter_aut,state_map,aut);

    // symbols (and arities)
    // symbols are added for nfta even if they are not enumerated, they are checked for validity so why not also add them
    assert(inter_aut->symbols_names.size() ==  inter_aut->symbols_arities.size() &&
               "The number of symbols and arities don't match");
    for (std::size_t i = 0; i < inter_aut->symbols_names.size(); i++) {
        alphabet->add_new_symbol(inter_aut->symbols_names[i], inter_aut->symbols_arities[i]);
    }

    // todo states can be enumerated too

    // transitions
    // if symbols are not enumerated, they are added when first encountered, arity is set to match the transition
    // symbols maybe added with multiple arities regardless of whether they are enumerated
    for (const auto& [formula_node, formula_graph] : inter_aut->transitions) {
        auto [source, symbol_str, targets] = get_transition(
            formula_node, formula_graph, state_map);
        const auto arity = static_cast<unsigned>(targets.size());
        const Symbol symbol = alphabet->translate_symbol(symbol_str, arity);
        aut.delta.add(source, symbol, targets);
    } // for transitions
    return aut;
} // construct_from_inter_aut wit Ranked alphabet

Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, Alphabet *alphabet) {
    // if is_alphabet_ranked is true, symbols are saved with arities
    // (separating alphabet and ranked alphabet might be a better idea)
    if (alphabet == nullptr) { static IntAlphabet ia; alphabet = &ia; }
    NameStateMap state_map = {};

    Nfta aut;
    aut.alphabet = alphabet;
    add_states(state_map, inter_aut->states_names, aut.delta);

    // both initial and final states are added to initial_states in nfta
    add_initial_and_final_states(inter_aut,state_map,aut);

    // symbols
    assert(inter_aut->symbols_names.size() ==  inter_aut->symbols_arities.size() &&
         "The number of symbols and arities don't match");
    for (const auto & symbols_name : inter_aut->symbols_names) {
        alphabet->translate_symb(symbols_name); // this should add
    }

    // transitions
    // if symbols are not enumerated, they are added when first encountered, arity is set to match the transition
    for (const auto& [formula_node, formula_graph] : inter_aut->transitions) {
        auto [source, symbol_str, targets] = get_transition(formula_node, formula_graph, state_map);
        Symbol symbol = alphabet->translate_symb(symbol_str); // this should throw
        aut.delta.add(source, symbol, targets);
    } // for transitions
    return aut;
} // construct_from_inter_aut

Nfta construct_from_parsed_object(const parser::Parsed *parsed, Alphabet *alphabet) {
    const IntermediateAut ia = IntermediateAut::parse_from_mf(*parsed)[0];
    return construct_from_inter_aut(&ia, alphabet);
}

Nfta parse_from_mata(std::istream& input, Alphabet *alphabet) {
    const parser::Parsed parsed = parser::parse_mf(input, true);
    const IntermediateAut ia = IntermediateAut::parse_from_mf(parsed)[0];
    if (RankedAlphabet *ranked; (ranked = dynamic_cast<RankedAlphabet*>(alphabet))) {
        return construct_from_inter_aut(&ia, ranked);
    }
    // if symbol overload is allowed, RankedOnTheFlyAlphabet must be used
    if (ia.overload) { throw std::runtime_error("Symbol overload requires a ranked alphabet."); }
    return construct_from_inter_aut(&ia, alphabet);
} // parse_from_mata

Nfta parse_from_mata(const std::string& input, Alphabet *alphabet) {
    std::istringstream in_stream(input);
    return parse_from_mata(in_stream, alphabet);
} // parse_from_mata

}// namespace mata::nfta
