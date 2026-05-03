/**
 * @file complementation.cc
 *
 * @brief Implementation of NFTA complementation operations.
 */

#include <mata/nfta/builder.hh>
#include "mata/nfta/nfta.hh"
#include <mata/nfta/utils.hh>


namespace mata::nfta {

Nfta complement(
        const Nfta& aut, const ParameterMap& params,
        const utils::OrdVector<SymbolArity>* symbols_arities_in) {
    if (!utils::haskey(params, "algorithm")) {
        throw std::runtime_error(
                std::to_string(__func__) +
                " requires setting the \"algorithm\" key in the \"params\" argument; "
                "received: " +
                std::to_string(params));
    }

    const std::string& str_algo = params.at("algorithm");
    if (str_algo == "top_down") {
        return complement_top_down(aut, nullptr, symbols_arities_in);
    } else if (str_algo == "classical") {
        const std::string str_det =
                utils::haskey(params, "determinization") ? params.at("determinization") : "optimized";

        Nfta det{};
        if (str_det == "optimized") {
            det = determinize_optimized(aut);
        } else if (str_det == "naive") {
            det = determinize_naive(aut);
        } else {
            throw std::runtime_error(
                    std::to_string(__func__) + " received an unknown value of the \"determinization\" key: " + str_det);
        }
        det.complement_as_deterministic(symbols_arities_in);
        return det;
    } else {
        throw std::runtime_error(
                std::to_string(__func__) + " received an unknown value of the \"algorithm\" key: " + str_algo);
    }
}


void Nfta::complement_as_deterministic(const utils::OrdVector<SymbolArity>* symbols_arities_in) {
    assert(is_bottom_up_deterministic() &&
           "mata::nft::complement_as_deterministic automaton is not bottom-up deterministic");
    if (root_states.empty() || delta.empty()) {
        utils::OrdVector<SymbolArity> tmp;
        const utils::OrdVector<SymbolArity>& symbols_arities = resolve_symbols_arities(*this, symbols_arities_in, tmp);
        *this = create_universal(&symbols_arities, alphabet);
    } else {
        make_complete(symbols_arities_in);
        swap_root_non_root();
        if (root_states.empty()) {
            *this = create_empty(alphabet);
        }
    }
}

Nfta complement_classical(const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities_in) {
    Nfta result = determinize_optimized(aut);

    result.complement_as_deterministic(symbols_arities_in);
    return result;
}


Nfta complement_top_down(
        const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping,
        const utils::OrdVector<SymbolArity>* symbols_arities_in) {

    utils::OrdVector<SymbolArity> tmp;
    const auto& symbols_arities = resolve_symbols_arities(aut, symbols_arities_in, tmp);

    Nfta result;
    result.alphabet = aut.alphabet;

    auto macrostate_mapping = make_mapping(aut, [&result]{ return result.delta.add_state(); }, state_mapping);

    if (aut.delta.empty() || aut.root_states.empty()) {
        return create_universal(&symbols_arities);
    }

    using Item = std::pair<State, StateSet>;
    struct Compare {
        bool operator()(const Item& a, const Item& b) const {
            return a.second.size() > b.second.size(); // sorted by state size
        }
    };
    std::priority_queue<Item, std::vector<Item>, Compare> worklist;

    auto get_or_create = [&](const StateSet& states) -> State {
        bool is_new = !macrostate_mapping.mapping->contains(states);
        State s = macrostate_mapping.get_or_create_macrostate(states);
        if (is_new)
            worklist.push({s, states});
        return s;
    };

    // initialize with the root macrostate
    const State q_det = get_or_create(StateSet(aut.root_states.begin(), aut.root_states.end()));
    result.add_root(q_det);

    // // component-wise subset
    // // returns 1 if a <= b (a subset eq of b), -1 if b < a (b subset of a), 0 if neither
    auto subset = [](const std::vector<StateSet>& a, const std::vector<StateSet>& b) -> int {
        assert(a.size() == b.size());
        bool a_sub_b = true;
        bool b_sub_a = true;
        for (size_t i = 0; i < a.size(); ++i) {
            auto it_a = a[i].begin(), end_a = a[i].end();
            auto it_b = b[i].begin(), end_b = b[i].end();
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
                if (!a_sub_b && !b_sub_a)
                    return 0;
            }
            if (it_a != end_a)
                a_sub_b = false;
            if (it_b != end_b)
                b_sub_a = false;
            if (!a_sub_b && !b_sub_a)
                return 0;
        }
        return a_sub_b ? 1 : -1;
    };

    while (!worklist.empty()) {
        auto [new_s, new_macro] = worklist.top();
        worklist.pop();

        for (const auto& [symbol, arity] : symbols_arities) {
            if (arity == 0) {
                bool leaf_accepts = true;
                for (const State q : new_macro) {
                    if (auto it = aut.delta[q].find(symbol); it != aut.delta[q].end() && !it->target_tuples.empty()) {
                        leaf_accepts = false;
                        break;
                    }
                }
                if (leaf_accepts) {
                    result.delta.add(new_s, symbol, {});
                }
                continue;
            }

            std::vector<std::vector<State>> constrains_vector;
            for (const State q : new_macro) {
                if (auto it = aut.delta[q].find(SymbolPost{symbol}); it == aut.delta[q].end()) {
                    continue;
                }
                for (auto& tup : aut.delta[q].find(symbol)->target_tuples) {
                    constrains_vector.push_back(tup);
                }
            }
            utils::OrdVector constrains(constrains_vector);
            utils::OrdVector<std::vector<State>> dominated;

            size_t m = constrains.size();
            std::cout << "constrains size: " << m << std::endl;
            std::vector<std::vector<State>> res_symbol_post_tmp;
            std::vector<unsigned> selector(m, 0);
            std::vector<std::vector<StateSet>> minimal_macro_tuples;

            unsigned cnt = 0;
            do {
                cnt++;
                std::vector<std::vector<State>> macro_tuple_tmp(arity);
                for (size_t t = 0; t < m; ++t) {
                    macro_tuple_tmp[selector[t]].push_back(constrains.at(t)[selector[t]]);
                }
                std::vector<StateSet> macro_tuple(arity);
                for (unsigned i = 0; i < arity; i++) {
                    macro_tuple[i] = utils::OrdVector(macro_tuple_tmp[i]);
                }

                bool is_redundant = false;
                for (auto it = minimal_macro_tuples.begin(); it != minimal_macro_tuples.end();) {
                    switch (subset(*it, macro_tuple)) {
                        case 1:
                            is_redundant = true;
                            break;
                        case -1:
                            it = minimal_macro_tuples.erase(it);
                            break;
                        default:
                            ++it;
                            break;
                    }
                    if (is_redundant) {
                        break;
                    }
                }
                if (!is_redundant)
                    minimal_macro_tuples.push_back(std::move(macro_tuple));
            } while (next_tuple(selector, arity));

            for (const auto& macro_tuple : minimal_macro_tuples) {
                std::vector<State> s_tuple(arity);
                for (unsigned i = 0; i < arity; ++i) {
                    s_tuple[i] = get_or_create(macro_tuple[i]);
                }
                res_symbol_post_tmp.push_back(s_tuple);
            }

            if (!res_symbol_post_tmp.empty()) {
                result.delta.mutable_state_post(new_s).push_back(
                        SymbolPost{symbol, utils::OrdVector(std::move(res_symbol_post_tmp))});
            }
        }
    }
    if (result.root_states.empty() || result.delta.empty()) {
        return create_empty(aut.alphabet);
    }
    return std::move(result);
} // complement_top_down

} // namespace mata::nfta
