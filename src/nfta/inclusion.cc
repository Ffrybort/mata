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
        return is_lang_included_antichains(smaller, bigger);
    }

    throw std::runtime_error(
            std::to_string(__func__) +
            " received an unknown value of the \"algorithm\" key: " + algo);
}

bool is_lang_included_antichains(const Nfta& smaller, const Nfta& bigger) {
    DeterminizeCache cache;

    //  1 -> a <= b
    // -1 -> b < a
    //  0 -> incomparable
    auto subset = [](const StateSet& a, const StateSet& b) -> int {
        bool a_sub_b = true;
        bool b_sub_a = true;
        auto it_a = a.begin(); const auto end_a = a.end();
        auto it_b = b.begin(); const auto end_b = b.end();
        while (it_a != end_a && it_b != end_b) {
            if (*it_a < *it_b) {
                a_sub_b = false;
                ++it_a;
            } else if (*it_b < *it_a) {
                b_sub_a = false;
                ++it_b;
            } else {
                ++it_a;
                ++it_b;
            }
            if (!a_sub_b && !b_sub_a) return 0;
        }
        if (it_a != end_a) a_sub_b = false;
        if (it_b != end_b) b_sub_a = false;
        if (a_sub_b) return 1;  // a <= b
        if (b_sub_a) return -1; // b < a
        return 0;
    };

    unsigned state_cnt = 0;
    auto macrostate_mapping = make_mapping(bigger, [&state_cnt]{ return state_cnt++; });
    const ReversedDelta bigger_rev_delta = bigger.delta.get_reversed();
    const ReversedDelta smaller_rev_delta = smaller.delta.get_reversed();
    ReversedDelta::RevSymbolPost dummy{}; // empty symbol post

    struct ProductPair {
        State smaller;
        State bigger;
        StateSet bigger_macro;

        ProductPair() = default;
        ProductPair(const State smaller, const State bigger, const StateSet& states)
            : smaller(smaller),
              bigger(bigger),
              bigger_macro(states)
        {}
    };

    auto compare = [](const ProductPair& a, const ProductPair& b) {
        return a.bigger_macro.size() > b.bigger_macro.size();
    };

    std::vector<ProductPair> worklist;
    std::vector<ProductPair> processed;

    // return true if inclusion holds so far, false it inclusion is broken
    auto process_new_pair_and_check_inclusion = [&](StateSet& smaller_states, const StateSet& bigger_states, BoolVector *useful_pairs) {
        // std::cout << "processing: " << smaller_states << " " << bigger_states << std::endl;
        if (smaller.root_states.intersects_with(smaller_states) && !bigger.root_states.intersects_with(bigger_states)) {
            return false;
        }
        const State bigger_det_state = macrostate_mapping.get_or_create_macrostate(bigger_states);

        for (unsigned i = 0; i < processed.size(); ++i) {
            State processed_smaller = processed[i].smaller;
            if (!smaller_states.contains(processed_smaller)) { continue; }
            int subset_res = subset(processed[i].bigger_macro, bigger_states);
            if (subset_res > 0) {
                // this pair is useless
                smaller_states.erase(processed_smaller);
            }
            if (subset_res < 0) {
                if (useful_pairs) { (*useful_pairs)[i] = false; }
            }
        }

        for (unsigned i = 0; i < worklist.size(); ++i) {
            State processed_smaller = worklist[i].smaller;
            if (!smaller_states.contains(processed_smaller)) { continue; }
            int subset_res = subset(worklist[i].bigger_macro, bigger_states);
            if (subset_res > 0) {
                smaller_states.erase(processed_smaller);
            }
            if (subset_res < 0) {
                worklist.erase(worklist.begin() + i);
                --i;
            }
        }

        // add the surviving pairs
        for (const State s : smaller_states) {
            worklist.push_back(ProductPair{s, bigger_det_state, bigger_states});
        }

        std::ranges::make_heap(worklist, compare);
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
            StateSet&smaller_states = sml_it->second; // individual states
            StateSet bigger_states{}; // a state set that will make a single det state

            // TODO: could there be any redundant states?
            if (!process_new_pair_and_check_inclusion(smaller_states, bigger_states, nullptr)) { return false; }
            ++sml_it;
         } else if (big_it->first < sml_it->first) {
             // symbol in bigger but not in smaller
             ++big_it;
         } else {
            // same symbol
            StateSet&smaller_states = sml_it->second; // individual states
            StateSet& bigger_states  = big_it->second; // a state set that will make a single det state

             // TODO: could there be any redundant states?
             if (!process_new_pair_and_check_inclusion(smaller_states, bigger_states, nullptr)) { return false; }

            ++big_it;
            ++sml_it;
        }
    }

    unsigned i = 0;
    while (!worklist.empty()) {
        i++;
        std::ranges::pop_heap(worklist, compare);
        ProductPair new_pair  = worklist.back();
        worklist.pop_back();

        processed.push_back(new_pair);

        // states are added to processed as they are discovered
        auto bigger_sp_it = bigger_rev_delta.symbol_posts.begin();
        for (auto &smaller_symbol_post : smaller_rev_delta.symbol_posts) {
            if (smaller_symbol_post.is_constant()) { continue; }
            Symbol symbol = smaller_symbol_post.symbol;
            Symbol arity = smaller_symbol_post.get_arity();

            while (bigger_sp_it->symbol < symbol) {
                // symbol is not in smaller (or it was a constant) -> skip to same or bigger
                ++bigger_sp_it;
            }
            if (bigger_sp_it->symbol > symbol) {
                // symbol in smaller only -> fill cache with an empty symbol post
                cache.fill(symbol, new_pair.bigger, arity, dummy, new_pair.bigger_macro);
            } else {
                // symbol in both -> fill cache
                const auto& bigger_symbol_post = *bigger_sp_it;
                assert(arity == bigger_symbol_post.get_arity());
                cache.fill(symbol, new_pair.bigger, arity, bigger_symbol_post, new_pair.bigger_macro);
                ++bigger_sp_it;
            }

            // iterate all tuples containing the new
            const size_t base = processed.size();
            const unsigned small_size = arity - 1;
            std::vector<unsigned> selector(small_size, 0);
            std::vector<ProductPair> small_tuple(small_size);
            std::vector<ProductPair> big_tuple(arity); // TODO: ptr or remove state set

            BoolVector state_pair_usefulness(processed.size(), true);
            size_t new_pair_index = processed.size() - 1;
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
                        smaller_tuple[i] = big_tuple[i].smaller;
                        bigger_tuple[i] = big_tuple[i].bigger;
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
                    if (!process_new_pair_and_check_inclusion(small_targets, bigger_targets, &state_pair_usefulness)) {
                        return false;
                    }
                    if (!state_pair_usefulness[new_pair_index]) { break; } // break from here and the whole do-while
                }
                if (!state_pair_usefulness[new_pair_index]) { break; }
            } while (next_tuple(selector, base));

            // remove useless state pair
            size_t write = 0;
            for (size_t read = 0; read < processed.size(); ++read) {
                if (state_pair_usefulness[read]) {
                    if (write != read) {
                        processed[write] = std::move(processed[read]);
                    }
                    ++write;
                }
            }
            processed.resize(write);
        }
    }
    return true;
}

}