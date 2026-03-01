#include "mata/nfta/nfta.hh"

namespace mata::nfta {
std::vector<StateSet> get_epsilon_closures(const Delta& delta, const Symbol epsilon) {
    const size_t num_of_states = delta.num_of_states();
    std::vector<StateSet> result{ num_of_states };
    std::vector<StateSet> epsilon_successors { num_of_states };

    // adding immediate successors
    for (State i = 0; i < static_cast<State>(num_of_states); i++) {
        result[i] = {i};
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
    const auto num_of_states = static_cast<unsigned>(delta.num_of_states());
    std::vector<StateSet> epsilon_closures = get_epsilon_closures(delta, epsilon);

    Nfta result { final_states, alphabet, Delta { num_of_states } };
    for (size_t i = 0; i < num_of_states; i++) {
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



}
