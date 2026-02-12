#include "mata/nfta/builder.hh"

namespace mata::nfta {

/**
 * @brief Get state from a string, insert to @p delta if not already in the @p state_map.
 */
static State get_state(const std::string& state_str, std::unordered_map<std::string, State>& state_map, Delta& delta) {
    auto [it, inserted] = state_map.try_emplace(state_str, State{});
    if (inserted) { it->second = delta.add_state(); }
    return it->second;
}

/**
 * @brief Add initial and final states as final states to nfta.
 */
void add_initial_and_final_states(const IntermediateAut* inter_aut, NameStateMap &state_map, Nfta &aut) {
    auto add_states = [&](const auto& states) {
        for (const auto& state_str : states) {
            State state = get_state(state_str, state_map, aut.delta);
            aut.add_final_state(state);
        }
    };
    add_states(inter_aut->initial_formula.collect_node_names());
    add_states(inter_aut->final_formula.collect_node_names());
}


/**
 * @brief Helper function to extract a transition from an inter_aut.
 * @param formula_node A node containing the left-hand side (source state).
 * @param formula_graph A graph containing the right-hand side (symbol and target(s)).
 * @param state_map A mapping of state names to numbers used by the constructor.
 * @param delta A delta of the automaton being constructed, new states are added.
 * @return A transition, with the symbol remaining a std::string, and states translated to internal numeric values.
 *
 * The reason for not translating a symbol right away is there is currently no unified way to do so for every possible
 * construction.
 */
std::tuple<State, std::string, std::vector<State>> get_transition(
    const FormulaNode &formula_node, const FormulaGraph &formula_graph, NameStateMap &state_map, Delta &delta) {
    State source = get_state(formula_node.name, state_map, delta);
    std::string symbol;
    std::vector<State> targets = {};
    if (formula_graph.node.is_and()) {
        symbol = formula_graph.children[0].node.name;
    }
    else {
        symbol = formula_graph.node.name;
    }

    std::size_t num_of_children = formula_graph.children.size();
    if (num_of_children == 2 && !formula_graph.children[1].node.is_and()) { // single target
        targets.push_back(get_state(formula_graph.children[1].node.name, state_map, delta));
    } else if (num_of_children == 2) {
        auto *tmp_graph = &formula_graph.children[1];
        num_of_children = tmp_graph->children.size();
        // walk through right side chain of & nodes
        while (num_of_children == 2 && tmp_graph->children[1].node.is_and())
        {
            if(!tmp_graph->children[1].node.is_symbol()) { // skip the symbol
                targets.push_back(get_state(tmp_graph->children[0].node.name, state_map, delta));
            }
            tmp_graph = &tmp_graph->children[1];
        }

        // final node - add both children
        if (num_of_children >= 1) {
            targets.push_back(
                get_state(tmp_graph->children[0].node.name, state_map, delta)
            );
        }
        if (num_of_children == 2) {
            targets.push_back(
                get_state(tmp_graph->children[1].node.name, state_map, delta)
            );
        }
    } // else if
    return {source, symbol, targets};
}

Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, RankedOnTheFlyAlphabet *alphabet) {
    // this might cause memory leaks if not handled right
    if (alphabet == nullptr) { alphabet = new RankedOnTheFlyAlphabet(); }
    Nfta aut;
    aut.alphabet = alphabet;
    NameStateMap state_map = {};

    // both initial and final states are added to final_states in nfta
    add_initial_and_final_states(inter_aut,state_map,aut);

    // symbols (and arities)
    if (inter_aut->are_symbols_enum_type()) {
        assert(inter_aut->symbols_names.size() ==  inter_aut->symbols_arities.size() &&
               "The number of symbols and arities don't match");
        for (std::size_t i = 0; i < inter_aut->symbols_names.size(); i++) {
            if (!inter_aut->overload &&alphabet->contains_symbol_name(inter_aut->symbols_names[i])) {
                // will throw if the arity does not match - duplicate symbols are fine
                alphabet->translate_ranked_symbol(inter_aut->symbols_names[i], inter_aut->symbols_arities[i]);
            }
            // otherwise add
            alphabet->translate_or_add_ranked_symbol(inter_aut->symbols_names[i], inter_aut->symbols_arities[i]);
        }
    }

    // transitions
    // if symbols are not enumerated, they are added when first encountered, arity is set to match the transition
    // symbols maybe added with multiple arities regardless of whether they are enumerated
    for (const auto& [formula_node, formula_graph] : inter_aut->transitions) {
        auto [source, symbol_str, targets] =
            get_transition(formula_node, formula_graph, state_map, aut.delta);

        Symbol symbol;
        auto arity = static_cast<unsigned>(targets.size());
        // a. symbols enumerated => all valid symbols have been added already
        // b. overload == true => adding every new symbol
        // c. overload = false AND the symbol name is already present => arity must match (duplicates are ok)
        // d. overload = false AND the symbol is new => add
        if (inter_aut->are_symbols_enum_type() || (!inter_aut->overload && alphabet->contains_symbol_name(symbol_str))) {
            // must be already in the alphabet with the correct arity
            symbol = alphabet->translate_ranked_symbol(symbol_str, arity);
        } else {
            // existing symbol is translated, new  is added
            symbol = alphabet->translate_or_add_ranked_symbol(symbol_str, arity);
        }
        aut.delta.add(source, symbol, std::move(targets));
    } // for transitions
    return aut;
} // construct_from_inter_aut wit Ranked alphabet

Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, Alphabet *alphabet) {
    // if is_alphabet_ranked is true, symbols are saved with arities
    // (separating alphabet and ranked alphabet might be a better idea)
    if (alphabet == nullptr) { static IntAlphabet ia; alphabet = &ia; }
    NameStateMap state_map = {};
    std::unordered_map<State, unsigned> symbol_arity_map = {};

    Nfta aut;
    aut.alphabet = alphabet;

    // both initial and final states are added to final_states in nfta
    add_initial_and_final_states(inter_aut,state_map,aut);

    // symbols (and arities)
    if (inter_aut->are_symbols_enum_type()) {
        assert(inter_aut->symbols_names.size() ==  inter_aut->symbols_arities.size() &&
               "The number of symbols and arities don't match");
        for (std::size_t i = 0; i < inter_aut->symbols_names.size(); i++) {
            symbol_arity_map[alphabet->translate_symb(inter_aut->symbols_names[i])] =  inter_aut->symbols_arities[i];
        }
    }

    // transitions
    // if symbols are not enumerated, they are added when first encountered, arity is set to match the transition
    for (const auto& [formula_node, formula_graph] : inter_aut->transitions) {
        auto [source, symbol_str, targets] = get_transition(formula_node, formula_graph, state_map, aut.delta);
        Symbol symbol = alphabet->translate_symb(symbol_str);
        // check valid arity
        if (symbol_arity_map.contains(symbol)) {
            if (targets.size() != symbol_arity_map[symbol]) {
                throw std::runtime_error("Arity of symbol " + std::to_string(symbol) + " does not match.");
            }
        } else { symbol_arity_map[symbol] = static_cast<unsigned>(targets.size()); }
        aut.delta.add(source, symbol, std::move(targets));
    } // for transitions
    return aut;
} // construct_from_inter_aut

Nfta construct_from_parsed_object(const parser::Parsed *parsed, Alphabet *alphabet) {
    IntermediateAut ia = IntermediateAut::parse_from_mf(*parsed)[0];
    return construct_from_inter_aut(&ia, alphabet);
}

Nfta parse_from_mata(std::istream& input, Alphabet *alphabet) {
    const parser::Parsed parsed = parser::parse_mf(input, true);
    const IntermediateAut ia = IntermediateAut::parse_from_mf(parsed)[0];
    if (RankedOnTheFlyAlphabet *ranked; (ranked = dynamic_cast<RankedOnTheFlyAlphabet*>(alphabet))) {
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
