#include "mata/nfta/builder.hh"



namespace mata::nfta {
Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, Alphabet *alphabet) {
    if (alphabet == nullptr) { throw std::runtime_error("A valid alphabet pointer is needed."); } // todo create one instead
    NameStateMap state_map;
    Nfta aut;
    aut.alphabet = alphabet;

    // get state numeric value from string, add it to state map if not already there
    auto get_state = [&state_map, &aut](const std::string& state) -> State {
        auto [it, inserted] = state_map.try_emplace(state, State{});
        if (inserted) {
            it->second = aut.delta.add_state();
        }
        return it->second;
    };

    // add initial states
    for (const auto& state_str : inter_aut->initial_formula.collect_node_names())
    {
        State state = get_state(state_str);
        aut.add_final_state(state);
    }

    // add final states
    for (const auto& state_str : inter_aut->final_formula.collect_node_names())
    {
        State state = get_state(state_str);
        aut.add_final_state(state);
    }

    // symbols and arities
    if (inter_aut->are_symbols_enum_type()) {
        assert(inter_aut->symbols_names.size() ==  inter_aut->symbols_arities.size() &&
               "The number of symbols and arities don't match");			for (std::size_t i = 0; i < inter_aut->symbols_names.size(); i++) {
                   aut.arities.set_arity(alphabet->translate_symb(inter_aut->symbols_names[i]), inter_aut->symbols_arities[i]);
               }
    }

    // transitions
    // if symbols are not enumerated, they are added when first encountered, arity is set to match the transition
    for (const auto& [formula_node, formula_graph] : inter_aut->transitions) {
        State source = get_state(formula_node.name);
        Symbol symbol;
        if (formula_graph.node.is_and()) {
            symbol = alphabet->translate_symb(formula_graph.children[0].node.name);
        }
        else {
            symbol = alphabet->translate_symb(formula_graph.node.name);
        }

        std::vector<State> targets = {};
        std::vector<Symbol> constants = {}; // for auto alphabet

        std::size_t num_of_children = formula_graph.children.size();
        if (num_of_children == 2 && !formula_graph.children[1].node.is_and()) { // single target
            targets.push_back(get_state(formula_graph.children[1].node.name));
        } else if (num_of_children == 2) {
            auto *tmp_graph = &formula_graph.children[1];
            num_of_children = tmp_graph->children.size();
            // walk through right side chain of & nodes
            while (num_of_children == 2 && tmp_graph->children[1].node.is_and())
            {
                if(!tmp_graph->children[1].node.is_symbol()) { // skip the symbol
                    targets.push_back(get_state(tmp_graph->children[0].node.name));
                }
                tmp_graph = &tmp_graph->children[1];
            }

            // final node - add both children
            if (num_of_children >= 1) {
                targets.push_back(
                    get_state(tmp_graph->children[0].node.name)
                );
            }
            if (num_of_children == 2) {
                targets.push_back(
                    get_state(tmp_graph->children[1].node.name)
                );
            }
        } // else if
        if (inter_aut->are_symbols_enum_type()) {
            if (targets.size() != aut.arities.get_arity(symbol)) {
                throw std::runtime_error(
                        "Symbol's arity needs to match the number of targets: " + std::to_string(symbol)); }
        } else {
            unsigned arity = aut.arities.get_arity(symbol);
            bool is_symbol_in_constants = std::ranges::find(constants, symbol) != constants.end();
            if (arity == targets.size()) {
                if (arity == 0) {
                    if (!is_symbol_in_constants) { constants.push_back(symbol); }
                }
            } else {
                if (is_symbol_in_constants) {
                    throw std::runtime_error("Arity mismatch in transitions." + std::to_string(symbol) ); }
                else { aut.arities.set_arity(symbol, static_cast<unsigned>(targets.size())); }
            }
        }
        aut.delta.add(source, symbol, targets);
    } // for transitions
    return aut;
} // construct_from_inter_aut

Nfta construct_from_parsed_object(const parser::Parsed *parsed, Alphabet *alphabet) {
    IntermediateAut ia = IntermediateAut::parse_from_mf(*parsed)[0];
    return construct_from_inter_aut(&ia, alphabet);
}

Nfta parse_from_mata(std::istream& input, Alphabet *alphabet) {
    parser::Parsed parsed = parser::parse_mf(input, true);
    IntermediateAut ia = IntermediateAut::parse_from_mf(parsed)[0];
    return construct_from_inter_aut(&ia, alphabet);
} // parse_from_mata

Nfta parse_from_mata(const std::string& input, Alphabet *alphabet) {
    std::istringstream in_stream(input);
    return parse_from_mata(in_stream, alphabet);
} // parse_from_mata
}