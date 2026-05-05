/**
 * NFTA-specific structures and helpers
 *
 */

#ifndef MATA_NFTA_UTILS_HH
#define MATA_NFTA_UTILS_HH
#include <mata/nfta/nfta.hh>
namespace mata::nfta {
struct DeterminizeCache { // TODO: move functions into some .cc
    using SymbolCache = std::vector<           // state
                    std::vector<               // position
                        utils::OrdVector<      // set of targets
                            const State*>>>;   // pointers into rev delta

    std::unordered_map<Symbol, SymbolCache> symbol_caches;

    bool fill(const Symbol symbol, const State new_s, const unsigned arity,
              const ReversedDelta::RevSymbolPost& symbol_post,
              const StateSet& new_macro)
    {
        // this function returned false if the resulting cache is empty - the state can be discarded
        // when called on an already nonempty symbol + state combination, nothing is done and true is returned
        auto& symbol_cache = symbol_caches[symbol];

        if (new_s >= symbol_cache.size()) { symbol_cache.resize(new_s + 1); }
        auto& new_s_cache = symbol_caches[symbol][new_s];
        if (!new_s_cache.empty()) { return true; }
        new_s_cache.resize(arity);

        bool is_nonempty = false;

        std::vector<std::vector<const State*>> collector(arity);
        for (const auto& tuple_post : symbol_post.state_tuple_posts) {
            for (unsigned i = 0; i < arity; i++) {
                if (new_macro.contains(tuple_post.sources[i])) {
                    collector[i].reserve(collector[i].size() + tuple_post.targets.size());
                    for (const State& t : tuple_post.targets) {
                        collector[i].push_back(&t);
                    }
                }
            }
        }

        for (unsigned i = 0; i < arity; i++) {
            if (!collector[i].empty()) {
                is_nonempty = true;
                new_s_cache[i] = utils::OrdVector(std::move(collector[i]));
            }
        }
        return is_nonempty;
    }

    utils::OrdVector<const State*>& operator()(Symbol sym, State s, unsigned pos) {
        return symbol_caches[sym][s][pos];
    }

    StateSet compute_targets(const Symbol symbol, const unsigned arity,
                         const std::vector<State>& big_tuple_s) {
        auto& symbol_cache = symbol_caches[symbol];

        std::vector<unsigned> order(arity);
        bool skip = false;
        for (unsigned i = 0; i < arity; i++) {
            assert(big_tuple_s[i] < symbol_cache.size() && !symbol_cache[big_tuple_s[i]].empty()
                && "empty cache");
            if (symbol_cache[big_tuple_s[i]][i].empty()) {
                skip = true;
                break; // no point continuing
                }
            order[i] = i;
        }
        if (skip) return utils::OrdVector<State>{};

        std::ranges::sort(order, [&](unsigned a, unsigned b) {
            return symbol_cache[big_tuple_s[a]][a].size() < symbol_cache[big_tuple_s[b]][b].size();
        });

        auto surviving = symbol_cache[big_tuple_s[order[0]]][order[0]];
        for (unsigned i : order) {
            surviving = surviving.intersection(symbol_cache[big_tuple_s[i]][i]);
        }

        std::vector<State> target_collector(surviving.size());
        for (unsigned i = 0; i < surviving.size(); i++) {
            target_collector[i] = *surviving.at(i);
        }
        return utils::OrdVector(target_collector);
    }
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
        s_to_macro.reserve(aut.delta.num_of_states()); // TODO: this should be optional
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
