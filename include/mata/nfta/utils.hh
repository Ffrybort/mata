/**
 * NFTA-specific structures and helpers
 *
 */


#ifndef MATA_NFTA_UTILS_HH
#define MATA_NFTA_UTILS_HH
#include <mata/nfta/nfta.hh>
namespace mata::nfta {
struct DeterminizeCache {
    using SymbolCache = std::vector<           // state
                    std::vector<               // position
                        utils::OrdVector<      // set of targets
                            const State*>>>;   // pointers into rev delta

    std::unordered_map<Symbol, SymbolCache> symbol_caches;

    void resize_for_state(Symbol symbol, State new_s, unsigned arity) {
        auto& sc = symbol_caches[symbol];
        if (new_s >= sc.size()) sc.resize(new_s + 1);
        sc[new_s].resize(arity);
    }

    bool fill(const Symbol symbol, const State new_s, const unsigned arity,
              const ReversedDelta::RevSymbolPost& symbol_post,
              const StateSet& new_macro)
    {
        resize_for_state(symbol, new_s, arity);
        auto& new_s_cache = symbol_caches[symbol][new_s];
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
};


// helper to resolve symbols and arities - use given or default
const utils::OrdVector<SymbolArity>& resolve_symbols_arities(
        const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities_in, utils::OrdVector<SymbolArity>& tmp);

// increment by 1 as a number with the given base, overflow => return false TODO: move this somewhere
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


struct MacrostateContext { // TODO: move this
    Nfta result{};
    std::unordered_map<StateSet, State>* mapping;
    std::vector<StateSet> s_to_macro;

    std::unordered_map<StateSet, State> local_mapping;

    explicit MacrostateContext(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping = nullptr)
        : result(), mapping(state_mapping ? state_mapping : &local_mapping), s_to_macro(), local_mapping() {
        result.alphabet = aut.alphabet;
        s_to_macro.reserve(aut.delta.num_of_states());
    }

    MacrostateContext(const MacrostateContext&) = delete;
    MacrostateContext& operator=(const MacrostateContext&) = delete;

    State get_or_create_macrostate(const StateSet& orig_states, const bool use_reversed_map = false) {
        assert(mapping);
        if (const auto it = mapping->find(orig_states); it != mapping->end()) {
            return it->second;
        }

        State new_s = result.delta.add_state();
        (*mapping)[orig_states] = new_s;

        if (use_reversed_map) {
            s_to_macro.resize(new_s + 1);
            s_to_macro[new_s] = orig_states;
        }

        return new_s;
    }
};
}
#endif //MATA_NFTA_UTILS_HH
