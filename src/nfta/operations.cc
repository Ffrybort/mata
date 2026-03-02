#include "mata/nfta/nfta.hh"
#include "mata/utils/two-dimensional-map.hh"

namespace mata::nfta {
std::vector<StateSet> get_epsilon_closures(const Delta& delta, const Symbol epsilon, const bool include_state = true) {
    const size_t num_of_states = delta.num_of_states();
    std::vector<StateSet> result{ num_of_states };
    std::vector<StateSet> epsilon_successors { num_of_states };

    // adding immediate successors
    for (State i = 0; i < static_cast<State>(num_of_states); i++) {
        if (include_state) { result[i] = {i}; }
        const auto& state_post = delta[i];
        const SymbolPost* symbol_post_ptr = nullptr;

        if (epsilon == EPSILON) {
            if (!state_post.empty() && state_post.back().symbol == epsilon) {
                symbol_post_ptr = &state_post.back();
            }
        } else  {
            const auto it = std::lower_bound(state_post.begin(), state_post.end(), epsilon,
                [](const SymbolPost& sp, const Symbol sym) { return sp.symbol < sym; } );
            if (it != state_post.end() && it->symbol == epsilon) { symbol_post_ptr = std::addressof(*it); }
        }
        if (symbol_post_ptr) {
            for (const auto& tr : symbol_post_ptr->target_tuples) {
                assert(tr.size() == 1);
                epsilon_successors[i].insert(tr.front());
                result[i].insert(tr.front());
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < num_of_states; ++i) {
            StateSet& src_eps_cl = result[i];
            for (const State tgt : epsilon_successors[i]) {
                const StateSet& tgt_eps_cl = result[tgt];
                changed = changed || src_eps_cl.insert(tgt_eps_cl);
            }
        }
    }
    return result;
}


void Nfta::remove_epsilon(const Symbol epsilon) {
    const auto num_of_states = static_cast<State>(delta.num_of_states());
    std::vector<StateSet> epsilon_closures = get_epsilon_closures(delta, epsilon);

    Nfta result { final_states, alphabet, Delta { num_of_states } };
    for (State i = 0; i < num_of_states; i++) {
        for (const State closure_of_i : epsilon_closures[i]) {
            if (is_state_final(closure_of_i)) { result.add_final_state(i); }
            for (const SymbolPost& symbol_post : delta[closure_of_i]) {
                if (symbol_post.symbol == epsilon) { continue; }
                result.delta.add(i, symbol_post);
            }
        }
    }
    *this = std::move(result);
}

void Nfta::remove_epsilon_in_place(const Symbol epsilon) {
    const auto num_of_states = static_cast<State>(delta.num_of_states());
    std::vector<StateSet> epsilon_closures = get_epsilon_closures(delta, epsilon, false);

    // remove epsilon transitions
    for (State i = 0; i < num_of_states; i++) {
        delta.try_remove(i, epsilon); // remove the entire symbol post
    }

    // add new transitions
    for (size_t i = 0; i < num_of_states; i++) {
        for (const State closure_of_i : epsilon_closures[i]) {
            if (is_state_final(closure_of_i)) { add_final_state(i); } // should work right?
            for (const SymbolPost& symbol_post : delta[closure_of_i]) {
                if (symbol_post.symbol == epsilon) { continue; }
                delta.add(i, symbol_post);
            }
        }
    }
}

void Nfta::union_nondet_in_place(const Nfta& aut) {
    const size_t orig_num_of_states{ delta.num_of_states() };
    const size_t aut_num_of_states{ aut.delta.num_of_states() };
    const size_t new_num_of_states{ orig_num_of_states + aut_num_of_states };

    if (this == &aut) { return; }

    if (final_states.empty()) { *this = aut; return; }
    if (aut.final_states.empty()) { return; }

    this->delta.reserve(new_num_of_states);

    auto renumber_states = [&](const State st) {
        return static_cast<State>(st + orig_num_of_states);
    };
    this->delta.append(aut.delta.renumber_targets(renumber_states));

    // Set accepting states.
    this->final_states.reserve(new_num_of_states);
    for(const State& aut_fin: aut.final_states) {
        this->final_states.insert(renumber_states(aut_fin));
    }
}

/// nfta must be epsilon free (for now)
Nfta union_product(const Nfta& A, const Nfta& B) {
    assert(A.is_deterministic());
    assert(B.is_deterministic());
    assert(A.is_complete());
    assert(B.is_complete());
    Nfta result;
    utils::TwoDimensionalMap<State> state_mapping{ A.delta.num_of_states(), B.delta.num_of_states() };
    std::deque<State> worklist{}; // Set of product states to process.

    auto final_condition = [&](const State state_A, const State state_B) {
        return A.is_state_final(state_A) || B.is_state_final(state_B);
    };

    auto create_product_state_and_symbol_post = [&](
        const std::vector<State>& targets_A, const std::vector<State>& targets_B, SymbolPost& product_symbol_post) {
        assert(targets_A.size() == targets_B.size());
        std::vector<State> result_targets{};
        result_targets.reserve(targets_A.size());
        for (size_t i = 0; i < targets_A.size(); i++) {
            State product_target = state_mapping.get(targets_A[i], targets_B[i] );
            if ( product_target == Limits::max_state) {
                product_target = result.delta.add_state();
                assert(product_target < Limits::max_state);
                state_mapping.insert(targets_A[i],targets_B[i], product_target);
                worklist.push_back(product_target);
                if (final_condition(targets_A[i], targets_B[i])) {
                    result.add_final_state(product_target);
                }
            }
        }
        //TODO: Push_back all of them and sort at the could be faster.
        product_symbol_post.insert(std::move(result_targets));
    };

    while (!worklist.empty()) {
        const State product_source = worklist.back();;
        worklist.pop_back();
        const State source_A = state_mapping.get_first_inverted(product_source);
        const State source_B = state_mapping.get_second_inverted(product_source);

        // Compute classic product for current state pair.
        utils::SynchronizedUniversalIterator<utils::OrdVector<SymbolPost>::const_iterator> sync_iterator(2);
        push_back(sync_iterator, A.delta[source_A]);
        push_back(sync_iterator, B.delta[source_B]);

        while (sync_iterator.advance()) {
            const std::vector<StatePost::const_iterator>& same_symbol_posts{ sync_iterator.get_current() };
            assert(same_symbol_posts.size() == 2); // One move per state in the pair.

            const Symbol symbol = same_symbol_posts[0]->symbol;
            SymbolPost product_symbol_post{ symbol };
            for (const auto& targets_A : same_symbol_posts[0]->target_tuples) {
                for (const auto& targets_B : same_symbol_posts[1]->target_tuples) {
                    create_product_state_and_symbol_post(targets_A, targets_B, product_symbol_post);
                }
            }
            StatePost &product_state_post{result.delta.mutable_state_post(product_source)};
            //Here we are sure that we are working with the largest symbol so far, since we iterate through
            //the symbol posts of the lhs and rhs in order. So we can just push_back (not insert).
            product_state_post.push_back(std::move(product_symbol_post));
        }
    }

    return result;
}

// Nfta intersection(const Nfta& A, const Nfta& B);

}
