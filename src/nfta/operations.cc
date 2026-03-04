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

    Nfta result { initial_states, alphabet, Delta { num_of_states } };
    for (State i = 0; i < num_of_states; i++) {
        for (const State closure_of_i : epsilon_closures[i]) {
            if (is_state_initial(closure_of_i)) { result.add_initial_state(i); }
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
    for (State i = 0; i < num_of_states; i++) {
        for (const State closure_of_i : epsilon_closures[i]) {
            if (is_state_initial(closure_of_i)) { add_initial_state(i); } // should work right?
            for (const SymbolPost& symbol_post : delta[closure_of_i]) {
                if (symbol_post.symbol == epsilon) { continue; }
                delta.add(i, symbol_post);
            }
        }
    }
}

void Nfta::union_nondet_in_place(const Nfta& aut) {
    if (this == &aut) { return; }
    if (initial_states.empty()) { *this = aut; return; }
    if (aut.initial_states.empty()) { return; }

    const size_t orig_num_of_states{ delta.num_of_states() };
    const size_t aut_num_of_states{ aut.delta.num_of_states() };
    const size_t new_num_of_states{ orig_num_of_states + aut_num_of_states };
    this->delta.reserve(new_num_of_states);

    auto renumber_states = [&](const State st) {
        return static_cast<State>(st + orig_num_of_states);
    };
    this->delta.append(aut.delta.renumber_targets(renumber_states));

    // Set accepting states.
    this->initial_states.reserve(new_num_of_states);
    for(const State& aut_fin: aut.initial_states) {
        this->initial_states.insert(renumber_states(aut_fin));
    }
}

Nfta union_nondet(const Nfta& A, const Nfta& B) {
    if (A.initial_states.empty() && B.initial_states.empty()) {return Nfta();}
    Nfta result{A}; result.union_nondet_in_place(B); return result;
}

/// nfta must be epsilon free
Nfta union_product(const Nfta& A, const Nfta& B) {
    assert(A.is_bottom_up_deterministic());
    assert(B.is_bottom_up_deterministic());
    assert(A.is_complete());
    assert(B.is_complete());

    if (A.initial_states.empty() || B.initial_states.empty()) { return union_nondet(A, B); }
    if (A == B) {return A; }

    auto result = product(A, B,[&](const State a, const State b){
            return A.is_state_initial(a) || B.is_state_initial(b); });
    return result;
}

template<typename FinalCondition>
Nfta product(const Nfta& A, const Nfta& B, FinalCondition&& condition) {
    Nfta result;
        utils::TwoDimensionalMap<State> state_mapping{ A.delta.num_of_states(), B.delta.num_of_states() };
    std::deque<State> worklist{}; // Set of product states to process.

    bool result_delta_empty = true;

    // Initialize pairs to process with final state pairs (initial states from top-down perspective)
    // todo is this correct for union?
    // union -> all possible combinations with at least one state final
    // intersection -> A.final x B.final
    for (const State initial_A : A.initial_states) {
        for (const State initial_B : B.initial_states) {
            // Update product with initial state pairs.
            const State product_initial_state = result.delta.add_state();
            state_mapping.insert(initial_A, initial_B, product_initial_state);
            worklist.push_back(product_initial_state);
            if (condition(initial_A, initial_B)) {
                result.initial_states.insert(product_initial_state);
            }
        }
    }

    auto create_product_state_and_symbol_post = [&](
        const std::vector<State>& targets_A, const std::vector<State>& targets_B, SymbolPost& product_symbol_post) {
        assert(targets_A.size() == targets_B.size());
        std::vector<State> result_targets{};
        result_targets.reserve(targets_A.size());
        for (size_t i = 0; i < targets_A.size(); i++) {
            State product_target = state_mapping.get(targets_A[i], targets_B[i] );
            if (product_target == Limits::max_state) {
                product_target = result.delta.add_state();
                assert(product_target < Limits::max_state);
                state_mapping.insert(targets_A[i],targets_B[i], product_target);
                worklist.push_back(product_target);
                if (condition(targets_A[i], targets_B[i])) {
                    result.add_initial_state(product_target);
                }

            }
            result_targets.push_back(product_target);
            result_delta_empty = false;
        }
        //TODO: Push_back all of them and sort later could be faster.
        product_symbol_post.insert(std::move(result_targets));
    };

    while (!worklist.empty()) {
        const State product_source = worklist.back();
        worklist.pop_back();
        const State source_A = state_mapping.get_first_inverted(product_source);
        const State source_B = state_mapping.get_second_inverted(product_source);

        utils::SynchronizedUniversalIterator<utils::OrdVector<SymbolPost>::const_iterator> sync_iterator(2);
        push_back(sync_iterator, A.delta[source_A]);
        push_back(sync_iterator, B.delta[source_B]);

        while (sync_iterator.advance()) {
            const std::vector<StatePost::const_iterator>& same_symbol_posts{ sync_iterator.get_current() };
            assert(same_symbol_posts.size() == 2);

            const Symbol symbol = same_symbol_posts[0]->symbol;
            SymbolPost product_symbol_post { symbol };
            for (const auto& targets_A : same_symbol_posts[0]->target_tuples) {
                for (const auto& targets_B : same_symbol_posts[1]->target_tuples) {
                    create_product_state_and_symbol_post(targets_A, targets_B, product_symbol_post);
                }
            }
            StatePost &product_state_post = result.delta.mutable_state_post(product_source);
            product_state_post.push_back(std::move(product_symbol_post));
        }
    }

    assert(result.delta.is_sorted());
    if (result_delta_empty) { return Nfta(); }
    return result;
}

Nfta intersection(const Nfta& A, const Nfta& B) {
    assert(A.is_complete());
    assert(B.is_complete());

    if (A.initial_states.empty() || B.initial_states.empty()) { return Nfta(); }
    if (A == B) {return A; }

    auto result = product(A, B,[&](const State a, const State b){
            return A.is_state_initial(a) && B.is_state_initial(b); });
    return result;
}

bool Nfta::is_bottom_up_deterministic() const {
    if (delta.is_empty()) { return true; }

    // todo this could be more efficient
    std::vector<Move> moves;

    for (const auto& state_post : delta) {
        for (const auto& symbol_post : state_post) {
            for (const auto& targets : symbol_post.target_tuples) {
                // every combination of symbol + target must be unique
                Move current  { symbol_post.symbol, targets };
                if (std::ranges::find(moves, current) != moves.end()) { return false; }
                moves.push_back( current );
            }
        }
    }
    return true;
}

bool Nfta::is_top_down_deterministic() const {
    if (initial_states.size() > 1) { return false; }
    if (delta.is_empty()) { return true; }

    const auto num_of_states = static_cast<State>(delta.num_of_states());
    for (State i = 0; i < num_of_states; ++i) {
        for (const auto& symbol_post : delta[i]) { if (symbol_post.target_tuples.size() != 1) { return false; } }
    }
    return true;
}

bool Nfta::is_complete() const {

    return true;
}


}
