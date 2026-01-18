#include "mata/nfta/builder.hh"


namespace mata::nfta {

    bool is_in_state_map(NameStateMap& map, const std::string & state) {
        return map.find(state) != map.end();
    }

    // adds a state to map and to nfta, unless it was already there, returns the state
    State get_state(Nfta& aut, NameStateMap& map, std::string state) {
        if (!is_in_state_map(map, state)) { map[state] = aut.add_state(); }
        return map[state];
    }
    Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, Alphabet *alphabet) {
        if (alphabet == nullptr) { throw std::runtime_error("A valid alphabet pointer is needed."); }
        NameStateMap state_map;
        Nfta aut(0, {}, {}, alphabet);
        if (!inter_aut->is_nfta_td()) { std::runtime_error("Expecting a top-down nfta"); }
        // add initial states
        for (const auto& state_str : inter_aut->initial_formula.collect_node_names())
        {
            State state = get_state(aut, state_map, state_str);
            aut.add_initial_state(state);
        }

        // symbols and arities
        if (inter_aut->are_symbols_enum_type()) {
            assert(inter_aut->symbols_names.size() ==  inter_aut->symbols_arities.size() && "The number of symbols and arities don't match");
			for (std::size_t i = 0; i < inter_aut->symbols_names.size(); i++) {
                aut.arities.set_arity(alphabet->translate_symb(inter_aut->symbols_names[i]), inter_aut->symbols_arities[i]);
            }
        }

        // transitions
        // if symbols are not enumerated, they are added when first encountered, arity is set to match the transition
        for (const auto& [formula_node, formula_graph] : inter_aut->transitions)
        {
            State source = get_state(aut, state_map, formula_node.name);
            Symbol symbol;
            if (formula_graph.node.is_and()) {
                symbol = alphabet->translate_symb(formula_graph.children[0].node.name);
            }
            else {
                symbol = alphabet->translate_symb(formula_graph.node.name);
                aut.add_transition(source, symbol, {});
                std::cout << "transition: " << source << " " << symbol << " ";
            std::cout    << std::endl;
                continue;
            }
            assert (formula_graph.children.size() == 2 && "something is very wrong here");

            std::vector<State> targets = {};
            std::vector<Symbol> constants = {}; // for auto alphabet

            if (!formula_graph.children[1].node.is_and()) {
                targets.push_back(get_state(aut, state_map, formula_graph.children[1].node.name));
            } else {
                auto *tmp_graph = &formula_graph.children[1];
                // walk through right side AND chain
                while (tmp_graph->children.size() == 2 && tmp_graph->children[1].node.is_and())
                {
                    if(!tmp_graph->children[1].node.is_symbol()) { // skip the symbol
                        targets.push_back(get_state(aut, state_map, tmp_graph->children[0].node.name));
                    }
                    tmp_graph = &tmp_graph->children[1];
                }

                // final node - add both children
                if (tmp_graph->children.size() >= 1) {
                    targets.push_back(
                        get_state(aut, state_map, tmp_graph->children[0].node.name)
                    );
                }
                if (tmp_graph->children.size() == 2) {
                    targets.push_back(
                        get_state(aut, state_map, tmp_graph->children[1].node.name)
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
            aut.add_transition(source, symbol, targets);
            std::cout << "transition: " << source << " " << symbol << " ";
            for (auto& t : targets) std::cout << t << " ";
            std::cout    << std::endl;
        } // for transitions
        return aut;
    } // construct_from_inter_aut

    Nfta parse_from_mata(std::istream& input) {
        parser::Parsed parsed = parser::parse_mf(input, true);
        IntermediateAut ia = IntermediateAut::parse_from_mf(parsed)[0];
        IntAlphabet alphabet;
        return construct_from_inter_aut(&ia, &alphabet);
    } // parse_from_mata


    Nfta parse_from_mata(const std::string& input) {
        std::istringstream in_stream(input);
        return parse_from_mata(in_stream);
    } // parse_from_mata

} // namespace mata::nfta::builder
