#include "mata/nfta/nfta.hh"
#include "mata/utils/two-dimensional-map.hh"
#include <cmath>
#include <bits/locale_facets_nonio.h>

namespace mata::nfta {
inline void unknown_symbol_in_delta(const std::optional<std::string> &symbol = std::nullopt) {
    if (symbol) {
        std::cerr << "Unknown symbol in delta: " << *symbol << std::endl;
    }
    assert(false && "Unknown symbol in delta");
} // unknown_symbol_in_delta

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
    } // for states in delta

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
} // get_epsilon_closures

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
} // remove_epsilon

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
} // remove_epsilon_in_place

void Nfta::swap_initial_states() {
    const auto num_of_states = static_cast<State>(delta.num_of_states());
    initial_states.complement(num_of_states);
}

Nfta complement(const Nfta& aut) {
    if (aut.initial_states.empty() || aut.delta.empty()) { return Nfta{}; }
    Nfta result = determinize_naive(aut);
    result.make_bottom_up_complete();
    result.swap_initial_states();
    return result;
}

void Nfta::unite_nondet_with(const Nfta& aut) {
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
} // unite_nondet_with

Nfta union_nondet(const Nfta& A, const Nfta& B) {
    if (A.initial_states.empty() && B.initial_states.empty()) {return Nfta();}
    Nfta result{A}; result.unite_nondet_with(B); return result;
} // union_nondet

/// nfta must be epsilon free
Nfta union_product(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out) {
    if (A.initial_states.empty() || B.initial_states.empty()) { return union_nondet(A, B); }
    if (A == B) {return A; }

    auto result = product(A, B, Condition::Or, state_mapping_out);
    return result;
} // union_product

/// automata must be top down complete over the same set of symbols
Nfta product(const Nfta& A, const Nfta& B, Condition cond, utils::TwoDimensionalMap<State> *state_mapping_out) {
    // todo this may be wrong, rewrite after consulting
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
    } // if union

    // a lambda to create a new product state,
    auto add_product_state = [&](const State source_A, const State source_B) {
        const State product_state = product.delta.add_state();
        state_mapping.insert(source_A, source_B, product_state);
        if (state_mapping_out) { state_mapping_out->insert(source_A, source_B, product_state); }

        if (cond == Condition::Or) { // union
            // if one of the sources has a leaf transition, the product state gets that transition
            product_leaf_tr.resize(product_state + 1);
            for (const Symbol symbol : leaf_tr_A[source_A]) { product_leaf_tr[product_state].push_back(symbol); }
            for (const Symbol symbol : leaf_tr_B[source_B]) { product_leaf_tr[product_state].push_back(symbol); }
        }
        return product_state;
    }; // add_product_state

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
    }; // create_product_state_and_symbol_post
    std::vector<std::pair<State, Symbol>> constant_tr_A = {};
    std::vector<std::pair<State, Symbol>> constant_tr_B = {};

    // get constant transitions first, then iterate and add product states
    // fuck this shit
    while (!worklist.empty()) { // todo now using a stack - would a queue be better?
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
    } // while worklist not empty
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
} // product

Nfta intersection(const Nfta& A, const Nfta& B) {
    if (A.initial_states.empty() || B.initial_states.empty()) { return Nfta(); }
    if (A == B) {return A; }

    auto result = product(A, B, Condition::And, nullptr);
    return result;
} // intersection

bool Nfta::is_bottom_up_deterministic() const {
    if (delta.empty()) { return true; }
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
} // is_bottom_up_deterministic

bool Nfta::is_top_down_deterministic() const {
    if (initial_states.size() > 1) { return false; }
    if (delta.empty()) { return true; }

    const auto num_of_states = static_cast<State>(delta.num_of_states());
    for (State i = 0; i < num_of_states; ++i) {
        for (const auto& symbol_post : delta[i]) { if (symbol_post.target_tuples.size() != 1) { return false; } }
    }
    return true;
} // is_top_down_deterministic

bool Nfta::is_bottom_up_complete(const utils::OrdVector<Symbol>& symbols) const { // todo test
    const size_t num_of_states = delta.num_of_states();
    ReversedDelta rev_delta = delta.get_reversed();
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
    } // for symbol transitions
    return true;
} // is_bottom_up_complete

bool Nfta::is_bottom_up_complete(const ReversedDelta& rev_delta) const { // todo test
    const size_t num_of_states = delta.num_of_states();
    for (const auto& symbol_tr : rev_delta.symbol_transitions) {
        assert(!symbol_tr.sources_transitions.empty() && "Source transitions not empty");
        const size_t arity = symbol_tr.sources_transitions.at(0).sources.size(); // todo use arity
        if (symbol_tr.sources_transitions.size() != ipow(num_of_states, arity)) { return false; }

        for (const auto& source_tr : symbol_tr.sources_transitions) {
            assert(source_tr.sources.size() == arity);
        }
    } // for symbol transitions
    return true;
} // is_bottom_up_complete

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
    } // for state posts
    return true;
} // is_top_down_complete

bool Nfta::is_top_down_complete() const {
    // for every state in delta, the number of symbol posts must be == to the number of non-constant symbols
    if (delta.num_of_states() == 0) { return true; }
    utils::OrdVector<Symbol> symbols;
    for (const auto& symbol_post : delta[0]) {
        if (!symbol_post.is_constant()) { symbols.push_back(symbol_post.symbol); }
    }
    for (const auto& state_post : delta) {
        unsigned n = 0; // counting present symbols
        for (const auto& symbol_post : state_post) {
            assert(!symbol_post.target_tuples.empty());
            if (symbols.contains(symbol_post.symbol)) { n++; }
            else if (!symbol_post.is_constant()) {
                return false;
            }
        }
        if (n != symbols.size()) {
            return false;
        }
    } // for state posts
    return true;
} // is_top_down_complete

// increment by 1 as a number with the given base, overflow => return false
bool next_tuple(std::vector<State>& tuple, const size_t base) {
    size_t pos = tuple.size();
    while (pos > 0) {
        --pos;
        if (++tuple[pos] < base) { return true; }
        tuple[pos] = 0;
    }
    return false;
} // next_tuple

// leave a given position alone
bool next_tuple_except(std::vector<State>& tuple, const size_t base, const unsigned position ) {
    assert(position < tuple.size() && "next_tuple_except: position out of range");
    size_t pos = tuple.size();

    while (pos > 0) {
        --pos;
        if (pos == position) { continue; }
        if (++tuple[pos] < base) { return true; }
        tuple[pos] = 0;
    }
    return false;
}

State get_sink(State sink, Delta& delta) {
    if (sink == Limits::max_state) { sink = delta.add_state(); }
    delta.resize_for_states(sink);
    return sink;
} // get_sink

void Nfta::make_bottom_up_complete(const utils::OrdVector<SymbolArity>& symbols_arities, State sink) {
    // todo leave it alone if it already was complete
    auto rev_delta = delta.get_reversed();

    // if the sink is default and delta is complete, it is left alone
    // todo it could have the correct number of symbols but wrong ones
    if (sink == Limits::max_state && rev_delta.symbol_transitions.size() == symbols_arities.size() &&
        is_bottom_up_complete(rev_delta)) { return; }

    sink = get_sink(sink, delta);
    const size_t num_of_states = delta.num_of_states();

    StatePost& sink_state_post = delta.mutable_state_post(sink);
    const bool sink_sp_empty = sink_state_post.empty();

    const bool delta_empty = delta.empty();

    auto rev_delta_it = rev_delta.symbol_transitions.begin();
    const auto rev_delta_end = rev_delta.symbol_transitions.end();

    auto input_symbols_it = symbols_arities.begin();
    const auto input_symbols_end = symbols_arities.end();

    // iterating over symbols - both in delta and in input
    while (rev_delta_it != rev_delta_end || input_symbols_it != input_symbols_end) {
        // symbol in delta that is not in the input
        if (rev_delta_it != rev_delta_end && (input_symbols_it == input_symbols_end || rev_delta_it->symbol < input_symbols_it->first)) {
            if (input_symbols_it == input_symbols_end) { std::cerr <<"Input symbols end" << std::endl; }
            unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(rev_delta_it->symbol));
            ++rev_delta_it;
            continue; // or ignore
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
            assert(symbol == rev_delta_it->symbol && "Symbols do not match when they should");
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
        } // else
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
        } // else
    } // while rev_delta_it != end OR input_symbols_it != end

    #ifndef NDEBUG
    assert(delta.is_sorted());
    const utils::OrdVector<Symbol> symbols = collect_symbols(symbols_arities);
    // assert(is_bottom_up_complete(symbols));
    #endif
} // make_bottom_up_complete

void Nfta::make_bottom_up_complete(const State sink ) { // todo ranked alph
    make_bottom_up_complete(delta.get_used_symbols_arities(), sink);
} // make_bottom_up_complete

void Nfta::make_top_down_complete(const State sink ) { // todo ranked alph
    make_top_down_complete(delta.get_used_symbols_arities(), sink);
} // make_top_down_complete

void Nfta::make_top_down_complete(const utils::OrdVector<SymbolArity>& symbols_arities, State sink) {
    // if the sink is default, automaton is checked for completeness before adding a sink state
    if (sink == Limits::max_state && is_top_down_complete(symbols_arities)) { return; } // todo
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
        } // while delta_symbols_it != end

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
    } // for states in delta

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

BoolVector Nfta::get_top_down_reachable() const {
    const size_t num_of_states = delta.num_of_states();
    BoolVector marked(num_of_states, false);
    std::deque<State> worklist{};

    worklist.insert(worklist.end(), initial_states.begin(), initial_states.end());
    for (const State state : initial_states) { marked[state] = true; }

    while (!worklist.empty()) {
        const State current_state = worklist.front();
        worklist.pop_front();
        for (const auto successor : delta.get_successors(current_state)) {
            if (!marked[successor]) {
                marked[successor] = true;
                worklist.push_back(successor);
            }
        }
    }
    return marked;
} // get_top_down_reachable

// todo general loop over reachable states could be implemented and shared
BoolVector Nfta::get_bottom_up_reachable() const {
    const ReversedDelta rev_delta = delta.get_reversed();
    const size_t num_of_states = delta.num_of_states();
    BoolVector marked(num_of_states, false);
    std::deque<State> worklist{};

    const auto bottom_up_initial = rev_delta.get_initial_states();
    for (const State state : bottom_up_initial) { marked[state] = true; }
    worklist.insert(worklist.end(), bottom_up_initial.begin(), bottom_up_initial.end());

    while (!worklist.empty()) {
        worklist.pop_front();

        for (const auto& sym_trans : rev_delta.symbol_transitions) {
            if (sym_trans.is_constant()) continue;
            for (const auto& src_tr : sym_trans.sources_transitions) {
                bool all_marked = true;
                for (const State s : src_tr.sources) { if (!marked[s]) { all_marked = false; break; } }
                if (!all_marked) { continue; }
                for (State target : src_tr.targets) {
                    if (!marked[target]) {
                        marked[target] = true;
                        worklist.push_back(target);
                    }
                }
            }
        }
    }
    return marked;
} // get_bottom_up_reachable

void Nfta::reduce_top_down() {
    const BoolVector marked = get_top_down_reachable();
    defragment(marked);
}
void Nfta::reduce_bottom_up_down() {
    const BoolVector marked = get_bottom_up_reachable();
    defragment(marked);
}

void Nfta::determinize(std::unordered_map<StateSet, State>* state_mapping) {
    *this = determinize_naive(*this, state_mapping);
}

// shared stuff between determinization versions
struct DeterminizationContext {
    Nfta& result;
    const Nfta& aut;

    std::unordered_map<StateSet, State>& state_mapping;
    std::vector<StateSet> det_state_to_sets;
    std::vector<std::pair<State, StateSet>>& worklist;

    State get_det_state(const StateSet& orig_states) {
        assert(!orig_states.empty());

        if (auto it = state_mapping.find(orig_states);
            it != state_mapping.end())
            return it->second;

        State q_det = result.delta.add_state();
        state_mapping[orig_states] = q_det;

        det_state_to_sets.resize(q_det + 1);
        det_state_to_sets[q_det] = orig_states;

        worklist.emplace_back(q_det, orig_states);

        if (aut.initial_states.intersects_with(orig_states))
            result.add_initial_state(q_det);

        return q_det;
    }
};

struct MacrostateConstructionContext {
    Nfta result{};
    std::unordered_map<StateSet, State>*mapping;
    std::vector<StateSet> det_to_macro;

    ReversedDelta rev_delta;
    std::vector<std::pair<State, StateSet>> worklist;
    const utils::SparseSet<State>& aut_initial_states;

    std::vector<State> processed_states; // already matched det states
    std::unordered_map<StateSet, State> local_mapping;

    explicit MacrostateConstructionContext(const Nfta& aut,
        std::unordered_map<StateSet, State>* state_mapping = nullptr)
        : result(),
          mapping(state_mapping ? state_mapping : &local_mapping),
          det_to_macro(),
          rev_delta(aut.delta.get_reversed()),
          worklist(),
          aut_initial_states(aut.initial_states),
          processed_states(),
          local_mapping()
    {
        result.alphabet = aut.alphabet;
        det_to_macro.reserve(aut.delta.num_of_states());
    }

    // delete copying
    MacrostateConstructionContext(const MacrostateConstructionContext&) = delete;
    MacrostateConstructionContext& operator=(const MacrostateConstructionContext&) = delete;


    // find or create det state from macro state
    // push to worklist if new
    State get_det_state(const StateSet& orig_states) {
        assert(!orig_states.empty());
        assert(mapping);
        if (const auto it = mapping->find(orig_states);
            it != mapping->end()) {
            return it->second;
            }

        State q_det = result.delta.add_state();
        (*mapping)[orig_states] = q_det;

        det_to_macro.resize(q_det + 1);
        det_to_macro[q_det] = orig_states;

        worklist.emplace_back(q_det, orig_states);
        if (aut_initial_states.intersects_with(orig_states)) { result.add_initial_state(q_det); }

        return q_det;
    }

    void initialize_with_bottom_up_initial() {
        // initialize with constant transitions
        for (auto bottom_up = rev_delta.get_initial_states_by_symbol();
             const auto& [symbol, states_orig] : bottom_up) {
            State q_det = get_det_state(states_orig);
            result.delta.add(q_det, symbol, {});
        }
    }
};

Nfta determinize_naive(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    MacrostateConstructionContext ctx(aut, state_mapping);
    if (aut.delta.empty()) { return ctx.result; }

    // initialize with constant transitions
    ctx.initialize_with_bottom_up_initial();

    // process reachable states
    while (!ctx.worklist.empty()) {
        auto [new_q, new_macro] = std::move(ctx.worklist.back());
        ctx.worklist.pop_back();
        assert(!new_macro.empty() && "determinize_naive: empty macro state in worklist");
        ctx.processed_states.push_back(new_q);

        for (const auto& symbol_tr : ctx.rev_delta.symbol_transitions) {
            // try to match every tuple containing the new state
            unsigned arity = symbol_tr.get_arity();
            if (arity == 0) { continue; }


            // generate tuples of size arity - 1, then insert the new state to each position
            const unsigned small_tuple_size = arity - 1;
            std::vector<State> index_tuple(small_tuple_size, 0); // incrementing indices
            const size_t base = ctx.processed_states.size();

            std::vector<State> small_tuple(small_tuple_size); // tuple of already processed det states
            std::vector<State> det_tuple(arity); // small tuple with new state inserted to some position

            do {
                // convert index tuple -> deterministic states
                for (unsigned i = 0; i < small_tuple_size; ++i) {
                    small_tuple[i] = ctx.processed_states[index_tuple[i]];
                }

                // insert new_q into every possible position
                for (unsigned pos = 0; pos < arity; ++pos) {
                    for (unsigned i = 0, k = 0; i < arity; ++i) {
                        det_tuple[i] = (i == pos) ? new_q : small_tuple[k++];
                    }

                    std::vector<State> targets;
                    // match transition sources and collect targets
                    for (const auto& src_tr: symbol_tr.sources_transitions) {
                        assert(src_tr.sources.size() == arity);
                        bool match = true;

                        for (unsigned i = 0; i < arity; ++i) {
                            if (const StateSet& macrostate = ctx.det_to_macro[det_tuple[i]];
                                !macrostate.contains(src_tr.sources[i])) {
                                match = false;
                                break;
                                }
                        }

                        if (match) { std::ranges::copy(src_tr.targets, std::back_inserter(targets)); }
                    }

                    // add a deterministic transition
                    if (targets.empty()) { continue; }
                    State q_target = ctx.get_det_state(utils::OrdVector<State>(targets));
                    ctx.result.delta.add(q_target, symbol_tr.symbol, det_tuple);
                }
            } while(next_tuple(index_tuple, base));
        }
    }

    assert(ctx.result.is_bottom_up_deterministic());
    return ctx.result;
}

Nfta determinize_optimized(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    struct SymbolCache {
        // [det_state][position] -> vector of targets sets
        std::vector< // state
            std::vector< // position
                StateSet
            > // targets
        > by_state;

        SymbolCache() : by_state() {}
    };

    MacrostateConstructionContext ctx(aut, state_mapping);

    if (aut.delta.empty()) { return ctx.result; }

    std::unordered_map<Symbol, SymbolCache> cache;
    cache.reserve(ctx.rev_delta.symbol_transitions.size());

    // initialize with constant transitions
    ctx.initialize_with_bottom_up_initial();

    // process reachable states
    while (!ctx.worklist.empty()) {
        auto [new_q, new_macro] = std::move(ctx.worklist.back());
        ctx.worklist.pop_back();
        assert(!new_macro.empty() && "determinize_optimized: empty macro state in worklist");
        ctx.processed_states.push_back(new_q);

        for (const auto& symbol_tr : ctx.rev_delta.symbol_transitions) {
            // try to match every tuple containing the new state
            Symbol symbol = symbol_tr.symbol;
            unsigned arity = symbol_tr.get_arity();
            if (arity == 0) { continue; }
            cache[symbol].by_state.resize(new_q + 1);
            auto& state_cache = cache[symbol].by_state[new_q];
            state_cache.resize(arity);


            for (const auto& src_tr: symbol_tr.sources_transitions) {
                for (unsigned i = 0; i < arity; ++i) {
                    // if sources[i] in macrostate -> add
                    if (new_macro.contains(src_tr.sources[i])) {
                        state_cache[i].insert(src_tr.targets);
                    }
                }
            }

            // generate tuples of size arity - 1, then insert the new state to each position
            const unsigned small_tuple_size = arity - 1;
            std::vector<State> index_tuple(small_tuple_size, 0); // incrementing indices
            const size_t base = ctx.processed_states.size();

            std::vector<State> small_tuple(small_tuple_size); // tuple of already processed det states
            std::vector<State> det_tuple(arity); // small tuple with new state inserted to some position

            do {
                // convert index tuple -> deterministic states
                for (unsigned i = 0; i < small_tuple_size; ++i) {
                    small_tuple[i] = ctx.processed_states[index_tuple[i]];
                }

                // insert new_q into every possible position
                for (unsigned pos = 0; pos < arity; ++pos) {
                    // new state appears in position i first time - all other states should be there
                    for (unsigned i = 0, k = 0; i < arity; ++i) {
                        det_tuple[i] = (i == pos) ? new_q : small_tuple[k++];
                    }

                    // intersect targets
                    StateSet targets;
                    bool first = true;
                    for (unsigned i = 0; i < arity; ++i) {
                        const StateSet& pos_targets = cache[symbol].by_state[det_tuple[i]][i];
                        if (first) {
                            targets = pos_targets;
                            first = false;
                        } else {
                            targets = StateSet::intersection(targets, pos_targets);
                        }
                        if (targets.empty()) { break; }
                    }

                    // add a deterministic transition
                    if (targets.empty()) { continue; }
                    State q_target = ctx.get_det_state(targets);
                    ctx.result.delta.add(q_target, symbol_tr.symbol, det_tuple);
                }
            } while(next_tuple(index_tuple, base));
        }
    }

    // print cache
    // for (const auto& [sym, c] : cache) {
    //     for (State q = 0; q < c.by_state.size(); ++q) {
    //         for (size_t i = 0; i < c.by_state[q].size(); ++i) {
    //             if (!c.by_state[q][i].empty()) {
    //                 std::cout << aut.alphabet->reverse_translate_symbol(sym) << " q" << q << " pos" << i
    //                           << " -> " << c.by_state[q][i] << "\n";
    //             }
    //         }
    //     }
    // }

    assert(ctx.result.is_bottom_up_deterministic());
    return ctx.result;
}


} // namespace mata::nfta
