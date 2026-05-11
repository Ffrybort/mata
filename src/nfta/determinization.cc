/**
 * @file determinization.cc
 *
 * @brief Implementation of NFTA determinization.
 */


#include <mata/nfta/builder.hh>
#include <mata/nfta/nfta.hh>
#include <mata/nfta/utils.hh>


namespace mata::nfta {
Nfta determinize(const Nfta& aut, const ParameterMap& params, std::unordered_map<StateSet, State>* state_mapping_out) {
    if (!utils::haskey(params, "algorithm")) {
        throw std::runtime_error(
                std::to_string(__func__) +
                " requires setting the \"algorithm\" key in the \"params\" argument; "
                "received: " +
                std::to_string(params));
    }

    const std::string& str_algo = params.at("algorithm");
    if (str_algo == "optimized") {
        return determinize_optimized(aut, state_mapping_out);
    }
    if (str_algo == "naive") {
        return determinize_naive(aut, state_mapping_out);
    }
    throw std::runtime_error(
            std::to_string(__func__) + " received an unknown value of the \"algorithm\" key: " + str_algo);
}

Nfta determinize_naive(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    return determinize_impl(
        aut, state_mapping, true,
        [](State, const StateSet&, const ReversedDelta::RevSymbolPost&, auto&) {
            return true;
        },
        [](const ReversedDelta::RevSymbolPost& symbol_post, const std::vector<State>& big_tuple_s,
           auto& ctx) -> StateSet {
            const unsigned arity = symbol_post.get_arity();
            StateSet targets;
            for (const auto& tuple_post : symbol_post.state_tuple_posts) {
                const std::vector<State>& tuple = tuple_post.sources;
                assert(tuple.size() == arity && "mata::nfta::determinize_naive arity mismatch");
                bool match = true;
                for (unsigned i = 0; i < arity; i++) {
                    if (!ctx.s_to_macro[big_tuple_s[i]].contains(tuple[i])) {
                        match = false;
                        break;
                    }
                }
                if (!match) continue;
                targets.insert(tuple_post.targets);
            }
            return targets;
        });
}

Nfta determinize_optimized(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    DeterminizeCache cache{};
    const ReversedDelta rev_delta = aut.delta.get_reversed();
    cache.symbol_caches.reserve(rev_delta.symbol_posts.size());

    auto on_new_state = [&](const State new_s, const StateSet& new_macro,
                            const ReversedDelta::RevSymbolPost& symbol_post, auto&) {
        return cache.fill(symbol_post.symbol, new_s, symbol_post.get_arity(), symbol_post, new_macro);
    };

    auto compute_targets = [&](const ReversedDelta::RevSymbolPost& symbol_post,
                           const std::vector<State>& big_tuple_s, auto&) -> StateSet {
        return cache.compute_targets(symbol_post.symbol, symbol_post.get_arity(), big_tuple_s);
    };

    return determinize_impl(aut, state_mapping, true, on_new_state, compute_targets);
}

template<typename OnNewState, typename ComputeTargets>
Nfta determinize_impl(
        const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping, const bool use_reverse_mapping,
        OnNewState on_new_state, ComputeTargets compute_targets) {

    Nfta result{};
    result.alphabet = aut.alphabet;

    auto ctx = make_mapping(aut, [&result]{ return result.delta.add_state(); }, state_mapping);

    const ReversedDelta rev_delta = aut.delta.get_reversed();

    using Item = std::pair<State, StateSet>;
    std::vector<Item> worklist;
    std::vector<State> processed;

    auto get_or_create = [&](const StateSet& states) -> State {
        bool is_new = !ctx.mapping->contains(states);
        State s = ctx.get_or_create_macrostate(states, use_reverse_mapping);
        if (is_new) {
            worklist.push_back({s, states});
            if (aut.root_states.intersects_with(states)) {
                result.add_root(s);
            }
        }
        return s;
    };

    for (auto [symbol, initial_states] : rev_delta.get_initial_states_by_symbol()) {
        const State new_s = get_or_create(initial_states);
        result.delta.add(new_s, symbol, {});
    }

    while (!worklist.empty()) {
        auto [new_s, new_macro] = std::move(worklist.back());
        worklist.pop_back();
        processed.push_back(new_s);

        // NOTE: Iterating over symbols first and then enumerating all possible tuples has proven to be faster, though
        // this has only been tested on moderate-size binary alphabets. Switching the do-while and for cycles (enumerating
        // tuples of a given arity ONCE for all symbols of that arity) *could* be faster for larger alphabets.
        for (const auto& symbol_post : rev_delta.symbol_posts) {
            const unsigned arity = symbol_post.get_arity();

            // constants are ignored, and a new state and symbol combination is optionally skipped
            if (arity == 0 || !on_new_state(new_s, new_macro, symbol_post, ctx)) { continue; }

            const size_t base = processed.size();
            const unsigned small_size = arity - 1;
            std::vector<unsigned> selector(small_size, 0);
            std::vector<State> small_tuple_s(small_size);
            std::vector<State> big_tuple_s(arity);
            do {
                for (unsigned i = 0; i < small_size; i++) {
                    small_tuple_s[i] = processed[selector[i]];
                }
                for (unsigned pos = 0; pos < arity; pos++) {
                    std::copy_n(small_tuple_s.begin(), pos, big_tuple_s.begin());
                    big_tuple_s[pos] = new_s;
                    std::copy(small_tuple_s.begin() + pos, small_tuple_s.end(), big_tuple_s.begin() + pos + 1);

                    StateSet targets = compute_targets(symbol_post, big_tuple_s, ctx);
                    if (targets.empty()) continue;

                    State target_s = get_or_create(targets);
                    result.delta.add(target_s, symbol_post.symbol, big_tuple_s);
                }
            } while (next_tuple(selector, base));
        }
    }

    if (result.root_states.empty() || result.delta.empty()) {
        return create_empty(result.alphabet);
    }
    return result;
}

} // namespace mata::nfta
