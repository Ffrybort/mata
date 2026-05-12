/**
 * @file nfta-utils.cc
 *
 * @brief Implementation of NFTA structures and helpers.
 *
 * Copyright (C) 2026, Felix Frybort.
 */

#include <mata/nfta/nfta-utils.hh>

using namespace mata;
namespace mata::nfta {
bool DeterminizeCache::fill(const Symbol symbol, const State new_s, const unsigned arity,
              const ReversedDelta::RevSymbolPost& symbol_post,
              const StateSet& new_macro) {
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

utils::OrdVector<const unsigned*>& DeterminizeCache::operator()(Symbol sym, State s, unsigned pos){
    return symbol_caches[sym][s][pos];
}

StateSet DeterminizeCache::compute_targets(const Symbol symbol, const unsigned arity,
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
}