#include <utility>
#include <mata/nfta/builder.hh>
#include "mata/nfta/nfta.hh"
#include <mata/nfta/utils.hh>

namespace mata::nfta {
bool is_lang_included(const Nfta& smaller, const Nfta& bigger, const ParameterMap& params) {
    if (!utils::haskey(params, "algorithm")) {
        throw std::runtime_error(
                std::to_string(__func__) +
                " requires setting the \"algorithm\" key in the \"params\" argument; "
                "received: " +
                std::to_string(params));
    }

    const std::string& algo = params.at("algorithm");

    if (algo == "naive") {
        // Build parameter map for complement
        ParameterMap complement_params;

        for (const auto& [key, value] : params) {
            if (key == "algorithm") continue;
            if (key == "complement") continue;
            complement_params[key] = value;
        }

        // Map "complement" -> "algorithm"
        if (utils::haskey(params, "complement")) {
            complement_params["algorithm"] = params.at("complement");
        } else {
            complement_params["algorithm"] = "classical";
        }

        Nfta bigger_compl = complement(bigger, complement_params);

        const Nfta inter = intersection(smaller, bigger_compl);
        return inter.is_lang_empty();
    }


    if (algo == "on-the-fly") {
        return is_lang_included_opt(smaller, bigger);
    }

    throw std::runtime_error(
            std::to_string(__func__) +
            " received an unknown value of the \"algorithm\" key: " + algo);
}

bool is_lang_included_opt(const Nfta& smaller, const Nfta& bigger) {
    DeterminizeCache cache;

    unsigned state_cnt = 0;
    auto macrostate_mapping = make_mapping(bigger, [&state_cnt]{ return state_cnt++; });
    const ReversedDelta bigger_rev_delta = bigger.delta.get_reversed();
    const ReversedDelta smaller_rev_delta = smaller.delta.get_reversed();
    ReversedDelta::RevSymbolPost dummy{}; // empty symbol post

    using StatePair = std::pair<State, State>;
    using WorklistItem = std::pair<StatePair, StateSet>;
    struct Compare {
        bool operator()(const WorklistItem& a, const WorklistItem& b) const {
            return a.second.size() > b.second.size(); // sorted by state size
        }
    };

    std::priority_queue<WorklistItem, std::vector<WorklistItem>, Compare> worklist; // todo how does this work actually

    std::vector<StatePair> processed;
    std::unordered_map<StatePair, size_t> processed_index; // todo could 2d map be used here?

    auto process_new_pair_and_check_inclusion = [&](const StateSet& smaller_states, StateSet& bigger_states) {
        if (smaller.root_states.intersects_with(smaller_states) && !bigger.root_states.intersects_with(bigger_states)) {
            return false;
        }

        State bigger_det_state = macrostate_mapping.get_or_create_macrostate(bigger_states);

        for (const auto s : smaller_states) {
            if (const StatePair pair { s, bigger_det_state }; !processed_index.contains(pair)) {
                worklist.push(WorklistItem{std::move(pair), bigger_states}); // todo check if contains
            }
        }
        return true;
    };

    // initialize with constant transitions
    auto bigger_init = bigger_rev_delta.get_initial_states_by_symbol();
    auto smaller_init = smaller_rev_delta.get_initial_states_by_symbol();

    // initialize with constant transitions
    auto big_it = bigger_init.begin();
    auto sml_it = smaller_init.begin();
    while (sml_it != smaller_init.end()) {
        if (big_it == bigger_init.end() || sml_it->first < big_it->first) {
            // symbol in smaller but not in bigger
            const StateSet& smaller_states = sml_it->second; // individual states
            StateSet bigger_states{}; // a state set that will make a single det state

            if (!process_new_pair_and_check_inclusion(smaller_states, bigger_states)) { return false; }
            ++sml_it;
         } else if (big_it->first < sml_it->first) {
             // symbol in bigger but not in smaller
             ++big_it;
         } else {
            // same symbol
            const StateSet& smaller_states = sml_it->second; // individual states
            StateSet& bigger_states  = big_it->second; // a state set that will make a single det state

             if (!process_new_pair_and_check_inclusion(smaller_states, bigger_states)) { return false; }

            ++big_it;
            ++sml_it;
        }
    }

    while (!worklist.empty()) {
        auto [new_pair, new_macro] = worklist.top();
        worklist.pop();
        if (processed_index.contains(new_pair)) { continue; }

        processed_index[new_pair] = processed.size();
        processed.push_back(new_pair);

        // states are added to processed as they are discovered
        auto bigger_sp_it = bigger_rev_delta.symbol_posts.begin();
        for (auto smaller_symbol_post : smaller_rev_delta.symbol_posts) {
            if (smaller_symbol_post.is_constant()) { continue; }
            Symbol symbol = smaller_symbol_post.symbol;
            Symbol arity = smaller_symbol_post.get_arity();
            bool is_symbol_present_in_bigger = false;
            while (bigger_sp_it->symbol < symbol) {
                // symbol is not in smaller (or it was a constant) -> skip to same or bigger
                ++bigger_sp_it;
            }
            if (bigger_sp_it->symbol > symbol) {
                // symbol in smaller only -> fill cache with an empty symbol post
                cache.fill(symbol, new_pair.second, arity, dummy, new_macro);
            } else {
                // symbol in both -> fill cache
                const auto& bigger_symbol_post = *bigger_sp_it;
                assert(arity == bigger_symbol_post.get_arity());
                cache.fill(symbol, new_pair.second, arity, bigger_symbol_post, new_macro);
            }

            // iterate all tuples containing the new
            const size_t base = processed.size();
            const unsigned small_size = arity - 1;
            std::vector<unsigned> selector(small_size, 0);
            std::vector<StatePair> small_tuple(small_size);
            std::vector<StatePair> big_tuple(arity);
            do {
                for (unsigned i = 0; i < small_size; i++) {
                    small_tuple[i] = processed[selector[i]];
                }
                for (unsigned pos = 0; pos < arity; pos++) {
                    std::copy_n(small_tuple.begin(), pos, big_tuple.begin()); // TODO: a lot of coppying - coud use pointers or smth
                    big_tuple[pos] = new_pair;
                    std::copy(small_tuple.begin() + pos, small_tuple.end(), big_tuple.begin() + pos + 1);

                    // extract smaller states from the big tuple
                    std::vector<State> smaller_tuple(arity);
                    std::vector<State> bigger_tuple(arity);
                    for (unsigned i = 0; i < arity; i++) {
                        smaller_tuple[i] = big_tuple[i].first;
                        bigger_tuple[i] = big_tuple[i].second;
                    }

                    // find matching source tuple in smaller symbol post
                    StateSet small_targets{}; // ptr?
                    if (const auto it = smaller_symbol_post.state_tuple_posts.find(
                            ReversedDelta::RevStateTuplePost{std::move(smaller_tuple)});
                            it != smaller_symbol_post.state_tuple_posts.end()) {
                        small_targets = it->targets;
                            }
                    if (small_targets.empty()) { continue; }

                    StateSet bigger_targets = cache.compute_targets(symbol, arity, bigger_tuple);

                    // if any of the smaller target is root and none of the bigger target is root, inclusion does not hold
                    if (!process_new_pair_and_check_inclusion(small_targets, bigger_targets)) { return false; }
                }
            } while (next_tuple(selector, base));
        }
    }
    return true;
}

}