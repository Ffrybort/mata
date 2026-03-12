#include "mata/nfta/nfta.hh"
#include "mata/utils/two-dimensional-map.hh"
#include <cmath>

namespace mata::nfta {

inline void unknown_symbol_in_delta(const std::optional<std::string> &symbol = std::nullopt) {
    if (symbol) {
        std::cerr << "Unknown symbol in delta: " << *symbol << std::endl;
    }
    assert(false && "Unknown symbol in delta");
}

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
Nfta union_product(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out) {
    if (A.initial_states.empty() || B.initial_states.empty()) { return union_nondet(A, B); }
    if (A == B) {return A; }

    auto result = product(A, B, Condition::Or, state_mapping_out);
    return result;
}

/// automata must be top down complete over the same set of symbols
Nfta product(const Nfta& A, const Nfta& B, Condition cond, utils::TwoDimensionalMap<State> *state_mapping_out) {
    // todo this is bullshit it was probably correct before
    #ifndef NDEBUG
    auto symbols = A.delta.get_used_symbols(true);
    symbols.insert(B.delta.get_used_symbols(true));
    assert(A.is_top_down_complete(symbols) && B.is_top_down_complete(symbols) &&
        "Automata must be top-down complete for product");
    #endif

    bool product_contains_leaf_tr = false;
    Nfta product;
    utils::TwoDimensionalMap<State> state_mapping{ A.delta.num_of_states(), B.delta.num_of_states() };
    std::deque<State> worklist{}; // Set of product states to process.

    std::vector<std::vector<Symbol>> leaf_tr_A(A.delta.num_of_states());
    std::vector<std::vector<Symbol>> leaf_tr_B(B.delta.num_of_states());
    std::vector<std::vector<Symbol>> product_leaf_tr;

    if (cond == Condition::Or) {
        // find all leaf transitions to add later
        for (State source = 0; source < A.delta.num_of_states(); source++) {
            for (const auto& symbol_post : A.delta[source]) {
                assert(!symbol_post.target_tuples.empty());
                if (symbol_post.is_constant()) { leaf_tr_A[source].emplace_back(symbol_post.symbol); }
            }
        }
        for (State source = 0; source < B.delta.num_of_states(); source++) {
            for (const auto& symbol_post : B.delta[source]) {
                assert(!symbol_post.target_tuples.empty());
                if (symbol_post.is_constant()) { leaf_tr_B[source].emplace_back(symbol_post.symbol); }
            }
        }
    }

    // a lambda to create a new product state,
    auto add_product_state = [&](const State source_A, const State source_B) {
        State product_state = product.delta.add_state();
        state_mapping.insert(source_A, source_B, product_state);
        if (state_mapping_out) { state_mapping_out->insert(source_A, source_B, product_state); }

        if (cond == Condition::Or) { // union
            // if one of the sources has a leaf transition, the product state gets that transition
            product_leaf_tr.resize(product_state + 1);
            for (const Symbol symbol : leaf_tr_A[source_A]) { product_leaf_tr[product_state].push_back(symbol); }
            for (const Symbol symbol : leaf_tr_B[source_B]) { product_leaf_tr[product_state].push_back(symbol); }
        }
        return product_state;
    };

    // Initialize worklist with initial state pairs
    for (const State initial_A : A.initial_states) {
        for (const State initial_B : B.initial_states) {
            // Update product with initial state pairs.
            const State product_initial_state = add_product_state(initial_A, initial_B);
            worklist.push_back(product_initial_state);
            product.initial_states.insert(product_initial_state);
        }
    }

    auto create_product_state_and_symbol_post = [&](
        const std::vector<State>& targets_A, const std::vector<State>& targets_B, SymbolPost& product_symbol_post) {
        assert(targets_A.size() == targets_B.size());
        if (targets_A.empty()) { product_contains_leaf_tr = true; }
        std::vector<State> result_targets{};
        result_targets.reserve(targets_A.size());
        for (size_t i = 0; i < targets_A.size(); i++) {
            State product_target = state_mapping.get(targets_A[i], targets_B[i] );
            if (product_target == Limits::max_state) {
                product_target = add_product_state(targets_A[i], targets_B[i]);
                worklist.push_back(product_target);
            }
            assert(product_target < Limits::max_state);
            result_targets.push_back(product_target);
        }
        //TODO: Push_back all of them and sort later could be faster.
        product_symbol_post.insert(std::move(result_targets));
    };
    std::vector<std::pair<State, Symbol>> constant_tr_A = {};
    std::vector<std::pair<State, Symbol>> constant_tr_B = {};

    // get constant tr first, then iterate and add product states
    // fuck this shit
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
            StatePost &product_state_post = product.delta.mutable_state_post(product_source);
            // adding constants before this means we need to insert and not push back
            product_state_post.push_back(std::move(product_symbol_post));
        }
    }
    if (cond == Condition::Or) {
        for (State source = 0; source < product_leaf_tr.size(); source++) {
            for (const auto& symbol : product_leaf_tr[source]) {
                product_contains_leaf_tr = true;
                product.delta.add(source, symbol, {});
            }
        }
    }
    product.alphabet = A.alphabet;
    assert(product.delta.is_sorted() && "Delta not sorted after product");
    if (!product_contains_leaf_tr) { return Nfta(); }
    return product;
}

Nfta intersection(const Nfta& A, const Nfta& B) {
    if (A.initial_states.empty() || B.initial_states.empty()) { return Nfta(); }
    if (A == B) {return A; }

    auto result = product(A, B, Condition::And, nullptr);
    return result;
}

bool Nfta::is_bottom_up_deterministic() const {
    if (delta.is_empty()) { return true; }
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

// todo symbols optional and default to alphabet?
bool Nfta::is_bottom_up_complete(const utils::OrdVector<Symbol>& symbols) const { // todo test
    const size_t num_of_states = delta.num_of_states();
    Delta::ReversedDelta rev_delta = delta.get_reversed();
    if (rev_delta.symbol_transitions.size() < symbols.size()) {
        return false;
    }
    for (const auto& symbol_tr : rev_delta.symbol_transitions) {
        if (!symbols.contains(symbol_tr.symbol)) {
            unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(symbol_tr.symbol));
        }
        assert(!symbol_tr.sources_transitions.empty() && "Source transitions not empty");
        const size_t arity = symbol_tr.sources_transitions.at(0).sources.size(); // todo use arity
        if (symbol_tr.sources_transitions.size() != ipow(num_of_states, arity)) { return false; }

        for (const auto& source_tr : symbol_tr.sources_transitions) {
            assert(source_tr.sources.size() == arity);
        }
    }
    return true;
}

// todo symbols optional and default to alphabet? that could be a problem with constants
/// symbols need to exclude constants
bool Nfta::is_top_down_complete(const utils::OrdVector<Symbol>& symbols) const {
    // for every state in delta, the number of symbol posts must be == to the number of non-constant symbols
    for (const auto& state_post : delta) {
        unsigned n = 0; // counting present symbols
        for (const auto& symbol_post : state_post) {
            assert(!symbol_post.target_tuples.empty());
            if (symbols.contains(symbol_post.symbol)) { n++; }
            else if (!symbol_post.is_constant()) {
                unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(symbol_post.symbol));
            }
        }
        if (n != symbols.size()) {
            return false;
        }
    }
    return true;
}

// increment by 1 as a number with the given base, overflow => return false
bool next_tuple(std::vector<State>& tuple, const size_t base) {
    size_t pos = tuple.size();
    while (pos > 0) {
        --pos;
        if (++tuple[pos] < base) { return true; }
        tuple[pos] = 0;
    }
    return false;
}

State get_sink(State sink, Delta& delta) {
    if (sink == Limits::max_state) { sink = delta.add_state(); }
    delta.resize_for_states(sink);
    return sink;
}

// todo some is_constant function in delta
void Nfta::make_bottom_up_complete(const utils::OrdVector<SymbolArity>& symbols_arities, State sink) {
    sink = get_sink(sink, delta);
    std::cout << "sink " << sink << std::endl;

    StatePost& sink_state_post = delta.mutable_state_post(sink);
    const bool sink_sp_empty = sink_state_post.empty();

    const bool delta_empty = delta.is_empty();
    auto rev_delta = delta.get_reversed();
    const size_t num_of_states = delta.num_of_states();

    auto rev_delta_it = rev_delta.symbol_transitions.begin();
    const auto rev_delta_end = rev_delta.symbol_transitions.end();

    auto input_symbols_it = symbols_arities.begin();
    const auto input_symbols_end = symbols_arities.end();

    // iterating over symbols - both in delta and in input
    while (rev_delta_it != rev_delta_end || input_symbols_it != input_symbols_end) {
        // symbol in delta that is not in the input
        if (!delta_empty && (input_symbols_it == input_symbols_end || rev_delta_it->symbol < input_symbols_it->first)) {
            unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(rev_delta_it->symbol));
            ++rev_delta_it; continue; // or ignore
        }
        const Symbol symbol = input_symbols_it->first;
        const unsigned arity = input_symbols_it->second;

        static const decltype(rev_delta_it->sources_transitions) empty{};
        auto source_tr_it  = empty.end();
        auto source_tr_end = empty.end();

        if (delta_empty || rev_delta_it == rev_delta_end || symbol < rev_delta_it->symbol) {
            ++input_symbols_it;
            // add all
        } else  { // symbols match
            assert(arity == rev_delta_it->sources_transitions.at(0).sources.size() && "Wrong arity");

            source_tr_it  = rev_delta_it->sources_transitions.cbegin();
            source_tr_end = rev_delta_it->sources_transitions.cend();

            const auto old_rev_delta_it = rev_delta_it;
            ++rev_delta_it;
            ++input_symbols_it;

            // if there are already all combinations, continue
            const size_t expected_num_of_transitions = ipow(num_of_states, arity);
            assert(old_rev_delta_it->sources_transitions.size() <= expected_num_of_transitions);
            if (old_rev_delta_it->sources_transitions.size() == expected_num_of_transitions) { continue; }
        }
        // adding transitions
        if (arity == 0) {
            sink_state_post.push_back(SymbolPost{ symbol, std::vector<State>{} });
        } else {
            std::vector<State> tuple(arity, 0);
            SymbolPost new_symbol_post {symbol};
            do {
                if (!delta_empty && source_tr_it != source_tr_end && source_tr_it->sources == tuple) {
                    ++source_tr_it; // tuple is already present
                } else {
                    // we are working with symbols in order, so push back is fine
                    new_symbol_post.push_back(tuple);
                }
            } while (next_tuple(tuple, num_of_states));
            if (sink_sp_empty) { sink_state_post.push_back(std::move(new_symbol_post)); }
            else { delta.add(sink, new_symbol_post); }
        }
    }

    #ifndef NDEBUG
    assert(delta.is_sorted());
    const utils::OrdVector<Symbol> symbols = collect_symbols(symbols_arities);
    assert(is_bottom_up_complete(symbols));
    #endif
}

//todo test
void Nfta::make_bottom_up_complete(State sink ) { // todo
    if (alphabet) { assert(false && "ranked alphabets not implemented yet"); }
    make_bottom_up_complete(delta.get_used_symbols_arities());
}

//todo test
void Nfta::make_top_down_complete(State sink ) { // todo
    if (alphabet) { assert(false && "ranked alphabets not implemented yet"); }
    make_top_down_complete(delta.get_used_symbols_arities());
}

void Nfta::make_top_down_complete(const utils::OrdVector<SymbolArity>& symbols_arities, State sink) {
    sink = get_sink(sink, delta);
    const auto num_of_states = static_cast<State>(delta.num_of_states());

    for (State state = 0; state < num_of_states; state++) {
        BoolVector symbols_found(symbols_arities.size(), false);
        size_t input_index = 0;
        auto delta_symbols_it = delta[state].cbegin();

        // iterating through symbols_arities and delta symbol posts at the same time, as both are ordered
        while (delta_symbols_it != delta[state].cend()) {
            if (input_index >= symbols_arities.size()) { // still some symbols in delta
                unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(delta_symbols_it->symbol));
                break;
            }
            const auto &[input_symbol, input_arity] = symbols_arities.at(input_index);
            assert(!delta_symbols_it->target_tuples.empty());

            // ignore constants
            if (delta_symbols_it->is_constant()) { ++delta_symbols_it; continue; }
            if (input_arity == 0)  { ++input_index; continue; }

            if (const Symbol delta_symbol = delta_symbols_it->symbol; input_symbol == delta_symbol) { // symbols match
                symbols_found[input_index] = true;
                ++delta_symbols_it;
                ++input_index;
            } else if (delta_symbol < input_symbol) {
                unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(delta_symbol));
                ++delta_symbols_it; // ignore it
            } else {
                // input symbol is not in this state post -> move to the next
                ++input_index;
            }
        }

        // add all non-constant symbols that were not found
        for (size_t j = 0; j < symbols_arities.size(); j++) {
            const Symbol symbol = symbols_arities.at(j).first;
            const unsigned arity = symbols_arities.at(j).second;
            if (arity == 0) { continue; }
            std::vector targets(arity, sink);
            if (!symbols_found[j]) {
                delta.add(state, symbol, targets);
            }
        }
    }

    // add sink loops
    StatePost sink_state_post = delta.mutable_state_post(sink);
    const bool sink_sp_empty = sink_state_post.empty();
    for (const auto &[symbol, arity] : symbols_arities) {
        if (arity > 0) {
            std::vector targets(arity, sink);
            if (sink_sp_empty) { sink_state_post.push_back(SymbolPost{ symbol, targets }); }
            else { delta.add(sink, symbol, targets); }
        } // could become unsorted if there was something already
    }

    #ifndef NDEBUG
    assert(delta.is_sorted());
    const utils::OrdVector<Symbol> symbols = collect_symbols(symbols_arities, true);
    assert(is_top_down_complete(symbols));
    #endif
} // make_top_down_complete

} // namespace mata::nfta
