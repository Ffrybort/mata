#include <mata/nfta/builder.hh>
#include <mata/nfta/nfta.hh>
#include "mata/utils/two-dimensional-map.hh"

namespace mata::nfta {
Nfta determinize(const Nfta& aut, const ParameterMap& params, std::unordered_map<StateSet, State>* state_mapping) {
    if (!utils::haskey(params, "algorithm")) {
        throw std::runtime_error(
                std::to_string(__func__) +
                " requires setting the \"algorithm\" key in the \"params\" argument; "
                "received: " +
                std::to_string(params));
    }

    const std::string& str_algo = params.at("algorithm");
    if (str_algo == "optimized") {
        return determinize_optimized(aut, state_mapping);
    }
    if (str_algo == "naive") {
        std::cout << "here\n";
        return determinize_naive(aut, state_mapping);
    }
    throw std::runtime_error(
            std::to_string(__func__) + " received an unknown value of the \"algorithm\" key: " + str_algo);
}

Nfta determinize_naive(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    return determinize_impl(
            aut, state_mapping, true,
            [](State, const StateSet&, const ReversedDelta::RevSymbolPost&, const MacrostateContext&) {
                return true;
            }, // use_reversed_map
            [](const ReversedDelta::RevSymbolPost& symbol_post, const std::vector<State>& big_tuple_s,
               const MacrostateContext& ctx) -> StateSet {
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
                    if (!match) {
                        continue;
                    }
                    targets.insert(tuple_post.targets);
                }
                return targets;
            });
}

Nfta determinize_optimized(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    using SymbolCache = std::vector< // state
            std::vector< // position
                    utils::OrdVector< // set of targets
                            const State* // pointers to rev delta
                            >>>;

    const ReversedDelta rev_delta = aut.delta.get_reversed();
    std::unordered_map<Symbol, SymbolCache> cache;
    cache.reserve(rev_delta.symbol_posts.size());

    auto on_new_state = [&](const State new_s, const StateSet& new_macro,
                            const ReversedDelta::RevSymbolPost& symbol_post, const MacrostateContext&) {
        const Symbol symbol = symbol_post.symbol;
        const unsigned arity = symbol_post.get_arity();

        auto& symbol_cache = cache[symbol];
        if (new_s >= symbol_cache.size()) {
            symbol_cache.resize(new_s + 1);
        }
        auto& new_s_cache = symbol_cache[new_s];
        new_s_cache.resize(arity);

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
            if (collector[i].empty()) {
                continue;
            }
            new_s_cache[i] = utils::OrdVector(std::move(collector[i]));
        }
        return true;
    };

    auto compute_targets = [&](const ReversedDelta::RevSymbolPost& symbol_post, const std::vector<State>& big_tuple_s,
                               const MacrostateContext&) -> StateSet {
        const Symbol symbol = symbol_post.symbol;
        const unsigned arity = symbol_post.get_arity();
        auto& symbol_cache = cache[symbol];

        std::vector<unsigned> order(arity);
        bool skip = false;
        for (unsigned i = 0; i < arity; i++) {
            if (symbol_cache[big_tuple_s[i]][i].empty()) {
                skip = true;
                continue;
            }
            order[i] = i;
        }
        if (skip) {
            return utils::OrdVector<State>{};
        }

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
    };

    return determinize_impl(aut, state_mapping, true, on_new_state, compute_targets);
}

template<typename OnNewState, typename ComputeTargets>
Nfta determinize_impl(
        const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping, const bool use_reverse_mapping,
        OnNewState on_new_state, ComputeTargets compute_targets) {

    MacrostateContext ctx(aut, state_mapping);
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
                ctx.result.add_root(s);
            }
        }
        return s;
    };

    // initialize with constant transitions
    for (auto [symbol, initial_states] : rev_delta.get_initial_states_by_symbol()) {
        const State new_s = get_or_create(initial_states);
        ctx.result.delta.add(new_s, symbol, {});
    }

    while (!worklist.empty()) {
        auto [new_s, new_macro] = std::move(worklist.back());
        worklist.pop_back();
        processed.push_back(new_s);

        for (const auto& symbol_post : rev_delta.symbol_posts) {
            if (symbol_post.is_constant()) {
                continue;
            }
            const unsigned arity = symbol_post.get_arity();

            if (!on_new_state(new_s, new_macro, symbol_post, ctx)) {
                continue;
            }

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

                    if (targets.empty()) {
                        continue;
                    }

                    State target_s = get_or_create(targets);
                    ctx.result.delta.add(target_s, symbol_post.symbol, big_tuple_s);
                }
            } while (next_tuple(selector, base));
        }
    }

    if (ctx.result.root_states.empty() || ctx.result.delta.empty()) {
        return create_empty(ctx.result.alphabet);
    }
    return std::move(ctx.result);
}

} // namespace mata::nfta
