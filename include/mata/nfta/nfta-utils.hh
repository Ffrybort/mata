/**
 * @file
 *
 * @brief Utility structures and helper functions for nondeterministic finite tree automata (NFTA).
 *
 * This file provides data structures and utility functions used by NFTA operations implemented in Mata.
 * It contains:
 *  - a caching structure supporting determinization procedures,
 *  - a mapping between macrostates and generated states.
 *  - a utility for resolving symbol-arity pairs,
 *  - a tuple iteration helper,
 *
 * The utilities defined here are primarily intended for internal algorithmic support and optimization.
 *
 * Copyright (C) 2026, Felix Frybort.
 */

#ifndef MATA_NFTA_UTILS_HH
#define MATA_NFTA_UTILS_HH

// mata headers
#include <mata/nfta/nfta.hh>

namespace mata::nfta {
struct DeterminizeCache {
    using SymbolCache = std::vector<           // state
                    std::vector<               // position
                        utils::OrdVector<      // set of targets
                            const State*>>>;   // pointers into rev delta

    DeterminizeCache() : symbol_caches() {}

    std::unordered_map<Symbol, SymbolCache> symbol_caches;

    bool fill(Symbol symbol, State new_s, unsigned arity,
              const ReversedDelta::RevSymbolPost& symbol_post,
              const StateSet& new_macro);

    utils::OrdVector<const State*>& operator()(Symbol sym, State s, unsigned pos);

    StateSet compute_targets(const Symbol symbol, const unsigned arity,
                         const std::vector<State>& big_tuple_s);
};


// helper to resolve symbols and arities - use given or default
const utils::OrdVector<SymbolArity>& resolve_symbols_arities(
        const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities_in, utils::OrdVector<SymbolArity>& tmp);

// increment by 1 as a number with the given base, overflow => return false
bool inline next_tuple(std::vector<State>& tuple, const size_t base) {
    size_t pos = tuple.size();
    while (pos > 0) {
        --pos;
        if (++tuple[pos] < base) {
            return true;
        }
        tuple[pos] = 0;
    }
    return false;
} // next_tuple

template<typename NewStateFn>
struct MacrostateMapping {
    std::unordered_map<StateSet, State>* mapping;
    std::vector<StateSet> s_to_macro;

    std::unordered_map<StateSet, State> local_mapping;
    NewStateFn new_state_fn;

    explicit MacrostateMapping(
        const Nfta& aut, NewStateFn new_state_fn, std::unordered_map<StateSet, State>* state_mapping = nullptr)

        : mapping(state_mapping ? state_mapping : &local_mapping), s_to_macro(), local_mapping(), new_state_fn(new_state_fn) {
        s_to_macro.reserve(aut.delta.num_of_states());
    }

    MacrostateMapping(const MacrostateMapping&) = delete;
    MacrostateMapping& operator=(const MacrostateMapping&) = delete;

    State get_or_create_macrostate(const StateSet& orig_states, const bool use_reversed_map = false) {
        assert(mapping);
        if (const auto it = mapping->find(orig_states); it != mapping->end()) {
            return it->second;
        }

        State new_s = new_state_fn();
        (*mapping)[orig_states] = new_s;

        if (use_reversed_map) {
            s_to_macro.resize(new_s + 1);
            s_to_macro[new_s] = orig_states;
        }

        return new_s;
    }
};

template<typename NewStateFn>
MacrostateMapping<NewStateFn> make_mapping(
        const Nfta& aut, NewStateFn fn,
        std::unordered_map<StateSet, State>* state_mapping = nullptr) {
    return MacrostateMapping<NewStateFn>(aut, std::move(fn), state_mapping);
}

}
#endif //MATA_NFTA_UTILS_HH
