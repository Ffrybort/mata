#include "mata/nfta/nfta.hh"
#include "mata/utils/two-dimensional-map.hh"
#include <cmath>
#include <mata/nfta/builder.hh>

#include "mata/nfta/ranked-alphabet.hh"

namespace mata::nfta {
inline void unknown_symbol_in_delta(const std::optional<std::string> &symbol = std::nullopt) {
    if (symbol) {
        std::cerr << "Unknown symbol in delta: " << *symbol << std::endl;
    }
    assert(false && "Unknown symbol in delta");
} // unknown_symbol_in_delta

// helper to resolve symbols and arities - use given or default
const utils::OrdVector<SymbolArity>& resolve_symbols_arities(
    const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities_in, utils::OrdVector<SymbolArity>& tmp
    ) {
    if (symbols_arities_in) { return *symbols_arities_in; }

    if (const auto* ranked = dynamic_cast<const RankedAlphabet*>(aut.alphabet)) {
        tmp = ranked->get_alphabet_symbols_arities();
        return tmp;
    }

    tmp = aut.delta.get_used_symbols_arities();
    return tmp;
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

Nfta remove_epsilon(const Nfta& aut, const Symbol epsilon) {
    const auto num_of_states = static_cast<State>(aut.delta.num_of_states());
    std::vector<StateSet> epsilon_closures = get_epsilon_closures(aut.delta, epsilon);

    Nfta result { aut.root_states, aut.alphabet, Delta { num_of_states } };
    for (State i = 0; i < num_of_states; i++) {
        for (const State closure_of_i : epsilon_closures[i]) {
            if (aut.is_state_root(closure_of_i)) { result.add_root(i); }
            for (const SymbolPost& symbol_post : aut.delta[closure_of_i]) {
                if (symbol_post.symbol == epsilon) { continue; }
                result.delta.add(i, symbol_post);
            }
        }
    }
    return result;
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
            if (is_state_root(closure_of_i)) { add_root(i); } // should work right?
            for (const SymbolPost& symbol_post : delta[closure_of_i]) {
                if (symbol_post.symbol == epsilon) { continue; }
                delta.add(i, symbol_post);
            }
        }
    }
} // remove_epsilon_in_place

void Nfta::complement_as_deterministic() {
    assert(is_bottom_up_deterministic() &&
        "mata::nfta::complement_as_deterministic automaton is not bottom-up deterministic");
    make_bottom_up_complete();
    swap_root_non_root();
}

Nfta complement_classical(const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities)  {
    if (aut.root_states.empty() || aut.delta.empty()) {
        return create_universal(symbols_arities, aut.alphabet);
    }
    Nfta result = determinize_optimized(aut);
    result.complement_as_deterministic();
    return result;
}

void Nfta::unite_nondet_with(const Nfta& aut) {
    if (this == &aut) { return; }
    if (root_states.empty()) { *this = aut; return; }
    if (aut.root_states.empty()) { return; }

    const size_t orig_num_of_states{ delta.num_of_states() };
    const size_t aut_num_of_states{ aut.delta.num_of_states() };
    const size_t new_num_of_states{ orig_num_of_states + aut_num_of_states };
    this->delta.reserve(new_num_of_states);

    auto renumber_states = [&](const State st) {
        return static_cast<State>(st + orig_num_of_states);
    };
    this->delta.append(aut.delta.renumber_targets(renumber_states));

    // Set accepting states.
    this->root_states.reserve(new_num_of_states);
    for(const State& aut_fin: aut.root_states) {
        this->root_states.insert(renumber_states(aut_fin));
    }
} // unite_nondet_with

Nfta union_nondet(const Nfta& A, const Nfta& B) {
    if (A.root_states.empty() && B.root_states.empty()) { return create_empty(A.alphabet); }
    Nfta result{ A };
    result.unite_nondet_with(B);
    return result;
} // union_nondet

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

// todo most of this is better to copy where is was
struct ProductContext {
    Nfta result;
    std::deque<State> worklist{}; // Set of product states to process.
    bool changed;
    utils::TwoDimensionalMap<State> *state_mapping;
    std::optional<utils::TwoDimensionalMap<State>> local_mapping;

    explicit ProductContext(const size_t num_states_A, const size_t num_states_B,
        utils::TwoDimensionalMap<State>* state_mapping_out)
        : result(), worklist(), changed(false), state_mapping(), local_mapping(std::nullopt) {
        if (state_mapping_out) {
            state_mapping = state_mapping_out;
        }
        else {
            local_mapping.emplace(num_states_A, num_states_B);
            state_mapping = &local_mapping.value();
        }
    }

    // get or create product state, push a new state into worklist
    State get_product_state_worklist (const State state_A, const State state_B) {
        State product_state = state_mapping->get(state_A, state_B );
        if (product_state == Limits::max_state) {
            // create new product state
            product_state = result.delta.add_state();
            state_mapping->insert(state_A, state_B, product_state);
            worklist.push_back(product_state);
        }
        assert(product_state < Limits::max_state);
        return product_state;
    }

    // get or create product state, set changed to true if a new state was created
    State get_product_state_fixpoint (const State state_A, const State state_B) {
        State product_state = state_mapping->get(state_A, state_B );
        if (product_state == Limits::max_state) {
            // create new product state
            product_state = result.delta.add_state();
            state_mapping->insert(state_A, state_B, product_state);
            changed = true;
        }
        assert(product_state < Limits::max_state);
        return product_state;
    }

    // initialize with pairs of initial states
    void initialize_top_down(const Nfta& A, const Nfta& B) {
        // Initialize worklist with initial state pairs
        for (const State initial_A : A.root_states) {
            for (const State initial_B : B.root_states) {
                // Update product with initial state pairs.
                const State product_initial_state = get_product_state_worklist(initial_A, initial_B);
                result.root_states.insert(product_initial_state);
            }
        }
    }

    // initialize with pairs of bottom-up initial states
    void initialize_bottom_up(
        const ReversedDelta &rev_delta_A, const ReversedDelta &rev_delta_B, const Nfta& A, const Nfta& B) {
        // for each constant symbol, pair up all states that accept it
        const auto const_tr_A = rev_delta_A.get_initial_states_by_symbol();
        const auto const_tr_B = rev_delta_B.get_initial_states_by_symbol();

        auto const_it_A = const_tr_A.begin();
        auto const_it_B = const_tr_B.begin();
        assert (const_tr_A.size() == const_tr_B.size() &&
            "mata::nfta Automata contain a different number of constant symbols");
        while (const_it_A != const_tr_A.end()) {
            const Symbol symbol_A = const_it_A->first;
            const Symbol symbol_B = const_it_B->first;
            assert(symbol_A == symbol_B && "mata::nfta Automata contain different constant symbols.");

            for (const State state_A : const_it_A->second) {
                for (const State state_B : const_it_B->second) {
                    const State product_state = get_product_state_fixpoint(state_A, state_B);
                    if (A.root_states.contains(state_A) || B.root_states.contains(state_B)) {
                        result.add_root(product_state);
                    }
                    result.delta.add(product_state, symbol_A, {});
                }
            }
            ++const_it_A;
            ++const_it_B;
        }
    }
    ProductContext(const ProductContext&) = delete;
    ProductContext& operator=(const ProductContext&) = delete;
};

Nfta union_det(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State>* state_mapping_out) {
    if (A.root_states.empty() && B.root_states.empty()) { return create_empty(); }

    const ReversedDelta rev_delta_A = A.delta.get_reversed();
    const ReversedDelta rev_delta_B = B.delta.get_reversed();
    assert(rev_delta_A.symbol_transitions.size() == rev_delta_B.symbol_transitions.size() &&
        "union_det: automata must have the same number of symbols");

    ProductContext ctx(A.delta.num_of_states(), B.delta.num_of_states(), state_mapping_out);
    ctx.initialize_bottom_up(rev_delta_A, rev_delta_B, A, B);

    // returns false if any child pair is unknown, otherwise fills source_tuple
    auto try_build_source_tuple = [&](const ReversedDelta::SourceTransitions& src_tr_A,
        const ReversedDelta::SourceTransitions& src_tr_B, std::vector<State>& source_tuple) -> bool {
        const size_t arity = src_tr_A.sources.size();
        assert(src_tr_B.sources.size() == arity);
        for (size_t i = 0; i < arity; ++i) {
            const State ps = ctx.state_mapping->get(src_tr_A.sources[i], src_tr_B.sources[i]);
            if (ps == Limits::max_state) return false;
            source_tuple[i] = ps;
        }
        return true;
    };

    auto process_target_pair = [&](const Symbol symbol, const std::vector<State>& source_tuple,
        const State target_A, const State target_B) {
        const State product_target = ctx.get_product_state_fixpoint(target_A, target_B);
        if (A.root_states.contains(target_A) || B.root_states.contains(target_B)) {
            ctx.result.add_root(product_target);
        }
        ctx.result.delta.add(product_target, symbol, source_tuple);
    };

    do {
        ctx.changed = false;
        auto sym_it_B = rev_delta_B.symbol_transitions.begin();

        for (const auto& sym_tr_A : rev_delta_A.symbol_transitions) {
            const auto& sym_tr_B = *sym_it_B++;
            if (sym_tr_A.is_constant()) continue;

            const Symbol symbol = sym_tr_A.symbol;
            const unsigned arity = sym_tr_A.get_arity();
            std::vector<State> source_tuple(arity);

            for (const auto& src_tr_A : sym_tr_A.sources_transitions) {
                for (const auto& src_tr_B : sym_tr_B.sources_transitions) {
                    if (!try_build_source_tuple(src_tr_A, src_tr_B, source_tuple)) { continue; }
                    for (const State target_A : src_tr_A.targets) {
                        for (const State target_B : src_tr_B.targets) {
                            process_target_pair(symbol, source_tuple, target_A, target_B);
                        }
                    }
                }
            }
        }
    } while (ctx.changed);
    assert(ctx.worklist.empty() && "mata::nfta::union_det worklist used when it ought not to be used");

    ctx.result.alphabet = A.alphabet;
    return ctx.result;
}

Nfta intersection(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out) {
    if (A.root_states.empty() || B.root_states.empty()) { return create_empty(); }

    #ifndef NDEBUG
    auto symbols = A.delta.get_used_symbols(true);
    symbols.insert(B.delta.get_used_symbols(true));
    assert(A.is_top_down_complete(symbols) && B.is_top_down_complete(symbols) &&
        "Automata must be top-down complete for product");
    #endif

    ProductContext ctx(A.delta.num_of_states(), B.delta.num_of_states(), state_mapping_out);
    ctx.initialize_top_down(A, B);

    // if no leaf transition has been added, an empty automaton is returned
    bool product_contains_leaf_tr = false;

    while (!ctx.worklist.empty()) {
        const State product_source = ctx.worklist.back();
        ctx.worklist.pop_back();
        const State source_A = ctx.state_mapping->get_first_inverted(product_source);
        const State source_B = ctx.state_mapping->get_second_inverted(product_source);

        utils::SynchronizedUniversalIterator<utils::OrdVector<SymbolPost>::const_iterator> sync_iterator(2);
        push_back(sync_iterator, A.delta[source_A]);
        push_back(sync_iterator, B.delta[source_B]);

        while (sync_iterator.advance()) {
            const std::vector<StatePost::const_iterator>& same_symbol_posts{ sync_iterator.get_current() };
            assert(same_symbol_posts.size() == 2);
            assert(same_symbol_posts[0]->symbol == same_symbol_posts[1]->symbol);

            const Symbol symbol = same_symbol_posts[0]->symbol;
            auto targets_tuples_A =  same_symbol_posts[0]->target_tuples;
            auto targets_tuples_B =  same_symbol_posts[1]->target_tuples;
            std::vector<std::vector<State>> product_symbol_post_tmp;
            product_symbol_post_tmp.reserve(targets_tuples_A.size() * targets_tuples_B.size());

            for (const auto& targets_A : targets_tuples_A) {
                for (const auto& targets_B : targets_tuples_B) {
                    assert(targets_A.size() == targets_B.size());
                    if (targets_A.empty()) { product_contains_leaf_tr = true; }
                    std::vector<State> result_targets{};
                    result_targets.reserve(targets_A.size());
                    for (size_t i = 0; i < targets_A.size(); i++) {
                        State product_target = ctx.get_product_state_worklist(targets_A[i], targets_B[i]);
                        result_targets.push_back(product_target);
                    }
                    product_symbol_post_tmp.push_back(std::move(result_targets));
                }
            }
            SymbolPost product_symbol_post { symbol, utils::OrdVector(std::move(product_symbol_post_tmp)) };
            StatePost &product_state_post = ctx.result.delta.mutable_state_post(product_source);
            // adding constants before this means we need to insert and not push back
            product_state_post.push_back(std::move(product_symbol_post));
        } // while sync_iterator.advance()
    } // while worklist not empty

    ctx.result.alphabet = A.alphabet;
    assert(ctx.result.delta.is_sorted() && "Delta not sorted after product");
    if (!product_contains_leaf_tr) { return create_empty(); }
    return ctx.result;
} // product

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
    if (root_states.size() > 1) { return false; }
    if (delta.empty()) { return true; }

    const auto num_of_states = static_cast<State>(delta.num_of_states());
    for (State i = 0; i < num_of_states; ++i) {
        for (const auto& symbol_post : delta[i]) { if (symbol_post.target_tuples.size() != 1) { return false; } }
    }
    return true;
} // is_top_down_deterministic

bool Nfta::is_bottom_up_complete() const {
    if (alphabet) {
        return is_bottom_up_complete(alphabet->get_alphabet_symbols());
    }
    return is_bottom_up_complete(delta.get_used_symbols());
}

bool Nfta::is_bottom_up_complete(const utils::OrdVector<Symbol> &symbols) const {
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
        const size_t arity = symbol_tr.get_arity();
        if (symbol_tr.sources_transitions.size() != ipow(num_of_states, arity)) { return false; }

        for (const auto& source_tr : symbol_tr.sources_transitions) {
            assert(source_tr.sources.size() == arity);
        }
    } // for symbol transitions
    return true;
} // is_bottom_up_complete

bool Nfta::is_top_down_complete(const utils::OrdVector<Symbol>& symbols) const {
    for (const auto s : symbols) {
        std::cout << " " << alphabet->try_reverse_translate_symbol(s);
    }
    std::cout << std::endl;
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
    if (const auto* ranked = dynamic_cast<RankedAlphabet*>(alphabet)) {
        return is_top_down_complete(ranked->get_non_constant_symbols());
    }
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

State get_sink(State sink, Delta& delta) {
    if (sink == Limits::max_state) { sink = delta.add_state(); }
    delta.resize_for_states(sink);
    return sink;
} // get_sink

void Nfta::make_bottom_up_complete(const utils::OrdVector<SymbolArity> *symbols_arities_in, State sink) {
    utils::OrdVector<SymbolArity> tmp;
    const utils::OrdVector<SymbolArity> symbols_arities = resolve_symbols_arities(*this, symbols_arities_in, tmp);

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

void Nfta::make_top_down_complete(const utils::OrdVector<SymbolArity> *symbols_arities_in, State sink) {
    // if the sink is default, automaton is checked for completeness before adding a sink state

    utils::OrdVector<SymbolArity> tmp;
    const utils::OrdVector<SymbolArity> symbols_arities = resolve_symbols_arities(*this, symbols_arities_in, tmp);
    if (sink == Limits::max_state && is_top_down_complete(symbols_arities)) { return; } // todo
    sink = get_sink(sink, delta);
    const auto num_of_states = static_cast<State>(delta.num_of_states());

    for (State state = 0; state < num_of_states; state++) {
        BoolVector symbols_found(symbols_arities.size(), false);

        // iterating through symbols_arities and delta symbol posts at the same time
        size_t input_index = 0;
        auto delta_symbols_it = delta[state].cbegin();

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

            if (const Symbol delta_symbol = delta_symbols_it->symbol; input_symbol == delta_symbol) {
                // symbols match
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
    sink_state_post.clear();
    for (const auto &[symbol, arity] : symbols_arities) {
        if (arity > 0) {
            std::vector targets(arity, sink);
            sink_state_post.push_back(SymbolPost{ symbol, targets });
        }
    }

    #ifndef NDEBUG
    assert(delta.is_sorted());
    const utils::OrdVector<Symbol> symbols = collect_symbols(symbols_arities, true);
    assert(is_top_down_complete(symbols));
    #endif
} // make_top_down_complete

BoolVector Nfta::get_top_down_reachable(const BoolVector *allowed) const {
    const size_t num_of_states = delta.num_of_states();
    BoolVector marked(num_of_states, false);
    std::deque<State> worklist{};

    auto mark = [&marked, allowed, &worklist](const State state) {
        if (!allowed || (*allowed)[state]) {
            marked[state] = true;
            worklist.push_back(state);
        }
    };

    worklist.insert(worklist.end(), root_states.begin(), root_states.end());
    for (const State state : root_states) { mark(state); }

    while (!worklist.empty()) {
        const State current_state = worklist.front();
        worklist.pop_front();
        for (const auto successor : delta.get_successors(current_state)) {
            if (!marked[successor]) {
                mark(successor);
            }
        }
    }
    return marked;
} // get_top_down_reachable

template<typename OnMarked>
BoolVector Nfta::get_bottom_up_reachable_impl(OnMarked&& early_exit_fn, const BoolVector *allowed) const {
    const ReversedDelta rev_delta = delta.get_reversed(allowed);
    const size_t num_of_states = delta.num_of_states();
    BoolVector marked(num_of_states, false);

    bool changed = false;
    auto mark = [&marked, allowed, &changed](const State state) {
        if (!allowed || (*allowed)[state]) {
            marked[state] = true;
            changed = true;
        }
    };

    for (const auto bottom_up_initial = rev_delta.get_initial_states();
        const State state : bottom_up_initial) {
        mark(state);
    }

    do {
        changed = false;
        for (const auto& sym_trans : rev_delta.symbol_transitions) {
            if (sym_trans.is_constant()) continue;
            for (const auto& src_tr : sym_trans.sources_transitions) {
                bool all_marked = true;
                for (const State s : src_tr.sources) { if (!marked[s]) { all_marked = false; break; } }
                if (!all_marked) { continue; }
                for (State target : src_tr.targets) {
                    if (!marked[target]) {
                        mark(target);
                        if (early_exit_fn(target, marked)) { return marked; }
                    }
                }
            }
        }
    } while (changed);

    return marked;
}

BoolVector Nfta::get_bottom_up_reachable(const BoolVector *allowed) const {
    return get_bottom_up_reachable_impl([](State, const BoolVector&) { return false; }, allowed);
}

bool Nfta::is_lang_empty() const {
    if (root_states.empty() || delta.empty()) return true;
    const BoolVector reachable = get_bottom_up_reachable_impl(
        [&](const State s, const BoolVector&) {
            return root_states.contains(s);
        }
    );
    for (const State s : root_states) {
        if (reachable[s]) { return false; }
    }
    return true;
}

void Nfta::reduce_top_down() {
    const BoolVector marked = get_top_down_reachable();
    defragment(marked);
}
void Nfta::reduce_bottom_up() {
    const BoolVector marked = get_bottom_up_reachable();
    defragment(marked);
}

void Nfta::reduce_top_bottom_top() {
    BoolVector marked = get_top_down_reachable();
    marked = get_bottom_up_reachable(&marked);
    marked = get_top_down_reachable(&marked);
    defragment(marked);
}

void Nfta::reduce_bottom_top() {
    BoolVector marked = get_bottom_up_reachable();
    marked = get_top_down_reachable(&marked);
    defragment(marked);
}

struct MacrostateConstructionContext {
    Nfta result{};
    std::unordered_map<StateSet, State>*mapping;
    std::vector<StateSet> s_to_macro;

    std::vector<std::pair<State, StateSet>> worklist;
    const utils::SparseSet<State>& aut_initial_states;

    std::vector<State> processed_states; // already matched det states
    std::unordered_map<StateSet, State> local_mapping;

    explicit MacrostateConstructionContext(const Nfta& aut,
        std::unordered_map<StateSet, State>* state_mapping = nullptr)
        : result(),
          mapping(state_mapping ? state_mapping : &local_mapping),
          s_to_macro(),
          worklist(),
          aut_initial_states(aut.root_states),
          processed_states(),
          local_mapping()
    {
        result.alphabet = aut.alphabet;
        s_to_macro.reserve(aut.delta.num_of_states());
    }

    // delete copying
    MacrostateConstructionContext(const MacrostateConstructionContext&) = delete;
    MacrostateConstructionContext& operator=(const MacrostateConstructionContext&) = delete;

    // find or create det state from macro state
    // push to worklist if new
    State get_or_create_macrostate(const StateSet& orig_states, const bool add_to_initial, const bool use_reversed_map = false) {
        assert(mapping);
        if (const auto it = mapping->find(orig_states); it != mapping->end()) {
            return it->second;
        }

        State new_s = result.delta.add_state();
        (*mapping)[orig_states] = new_s;

        if (use_reversed_map) {
            s_to_macro.resize(new_s + 1);
            s_to_macro[new_s] = orig_states;
        }

        worklist.emplace_back(new_s, orig_states);
        if (add_to_initial && aut_initial_states.intersects_with(orig_states)) { result.add_root(new_s); }

        return new_s;
    }

    // initialize the worklist and result with constant transitions (used in determinizing)
    void initialize_bottom_up(const ReversedDelta& rev_delta) {
        // initialize with constant transitions
        for (auto bottom_up = rev_delta.get_initial_states_by_symbol();
            const auto& [symbol, states_orig] : bottom_up) {
            const State new_s = get_or_create_macrostate(states_orig, true, true);
            result.delta.add(new_s, symbol, {});
        }
    }

    // initialize the worklist and result with an initial macrostate
    void initialize_top_down() {
        // initialize with constant transitions
        const State q_det = get_or_create_macrostate(StateSet(aut_initial_states.begin(),
            aut_initial_states.end()),  false);
        result.add_root(q_det);
    }
};

Nfta determinize_naive(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping) {
    MacrostateConstructionContext ctx(aut, state_mapping);
    if (aut.delta.empty()) { return ctx.result; }

    ReversedDelta rev_delta = aut.delta.get_reversed();
    // initialize with constant transitions
    ctx.initialize_bottom_up(rev_delta);

    // process reachable states
    while (!ctx.worklist.empty()) {
        auto [new_s, new_macro] = std::move(ctx.worklist.back());
        ctx.worklist.pop_back();
        assert(!new_macro.empty() && "determinize_naive: empty macro state in worklist");
        ctx.processed_states.push_back(new_s);

        for (const auto& symbol_tr : rev_delta.symbol_transitions) {
            // try to match every tuple containing the new state
            const unsigned arity = symbol_tr.get_arity();
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

                // insert new_s into every possible position
                for (unsigned pos = 0; pos < arity; ++pos) {
                    for (unsigned i = 0, k = 0; i < arity; ++i) {
                        det_tuple[i] = (i == pos) ? new_s : small_tuple[k++];
                    }

                    std::vector<State> targets;
                    // match transition sources and collect targets
                    for (const auto& src_tr: symbol_tr.sources_transitions) {
                        assert(src_tr.sources.size() == arity);
                        bool match = true;

                        for (unsigned i = 0; i < arity; ++i) {
                            if (const StateSet& macrostate = ctx.s_to_macro[det_tuple[i]];
                                !macrostate.contains(src_tr.sources[i])) {
                                match = false;
                                break;
                            }
                        }

                        if (match) { std::ranges::copy(src_tr.targets, std::back_inserter(targets)); }
                    }

                    // add a deterministic transition
                    if (targets.empty()) { continue; }
                    const State q_target = ctx.get_or_create_macrostate(StateSet(targets), true, true);
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

    ReversedDelta rev_delta = aut.delta.get_reversed();
    MacrostateConstructionContext ctx(aut, state_mapping);

    if (aut.delta.empty()) { return ctx.result; }

    std::unordered_map<Symbol, SymbolCache> cache;
    cache.reserve(rev_delta.symbol_transitions.size());

    // initialize with constant transitions
    ctx.initialize_bottom_up(rev_delta);

    // process reachable states
    while (!ctx.worklist.empty()) {
        auto [new_s, new_macro] = std::move(ctx.worklist.back());
        ctx.worklist.pop_back();
        assert(!new_macro.empty() && "determinize_optimized: empty macro state in worklist");
        ctx.processed_states.push_back(new_s);

        for (const auto& symbol_tr : rev_delta.symbol_transitions) {
            // try to match every tuple containing the new state
            Symbol symbol = symbol_tr.symbol;
            unsigned arity = symbol_tr.get_arity();
            if (arity == 0) { continue; }
            cache[symbol].by_state.resize(new_s + 1);
            auto& state_cache = cache[symbol].by_state[new_s];
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

                // insert new_s into every possible position
                for (unsigned pos = 0; pos < arity; ++pos) {
                    // new state appears in position i first time - all other states should be there
                    for (unsigned i = 0, k = 0; i < arity; ++i) {
                        det_tuple[i] = (i == pos) ? new_s : small_tuple[k++];
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
                    State q_target = ctx.get_or_create_macrostate(targets, true, true);
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

Nfta complement_top_down(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping,
        const utils::OrdVector<SymbolArity>* symbols_arities_in) {
    // todo does it have to be complete?

    utils::OrdVector<SymbolArity> tmp;
    const auto& symbols_arities = resolve_symbols_arities(aut, symbols_arities_in, tmp);
    MacrostateConstructionContext ctx(aut, state_mapping);

    for (const auto& [symbol, arity] : symbols_arities) {
        std::cout << aut.alphabet->try_reverse_translate_symbol(symbol) << ":" << arity << '\n';
    }

    if (aut.delta.empty() || aut.root_states.empty()) { return create_universal(&symbols_arities); }
    ctx.initialize_top_down();

    // component-wise subset: returns true iff a is dominated by b (b ≤ a, i.e. b is smaller-or-equal)
    auto is_subset = [](const std::vector<StateSet>& small, const std::vector<StateSet>& big) -> bool {
        for (size_t i = 0; i < big.size(); ++i) {
            // small is subset of big iff every component of small is big subset of the corresponding component of big
            if (!std::ranges::includes(big[i],small[i]))
                return false;
        }
        return true;
    };

    while (!ctx.worklist.empty()) {
        auto [new_s, new_macro] = std::move(ctx.worklist.back());
        ctx.worklist.pop_back();

        for (const auto& [symbol, arity] : symbols_arities) {
            // constant -> if there are no transitions over symbol from any q, add to result
            if (arity == 0) {
                bool leaf_accepts = true;
                for (const State q : new_macro) {
                    if (auto it = aut.delta[q].find(symbol); it != aut.delta[q].end() && !it->target_tuples.empty()) {
                        leaf_accepts = false;
                    }
                }
                if (leaf_accepts) {
                    ctx.result.delta.add(new_s, symbol, {});
                }
                continue;
            }

            std::vector<std::vector<State>> constrains_vector; // all target tuples of any q over symbol
            for (const State q : new_macro) {
                auto it = aut.delta[q].find(SymbolPost{symbol});
                assert(it != aut.delta[q].end() && "mata::nfta::complement_top_down missing symbol post");
                for (auto& tup : aut.delta[q].find(symbol)->target_tuples) {
                    constrains_vector.push_back(tup);
                }
            }
            utils::OrdVector constrains(constrains_vector);

            size_t m = constrains.size();
            SymbolPost res_symbol_post(symbol);

            std::vector<unsigned> selector(m, 0);
            std::vector<std::vector<StateSet>> minimal_macro_tuples;

            do {
                std::vector<StateSet> macro_tuple(arity);
                for (size_t t = 0; t < m; ++t)
                    macro_tuple[selector[t]].insert(constrains.at(t)[selector[t]]);

                bool is_redundant = false;
                for (const auto& existing : minimal_macro_tuples) {
                    if (is_subset(existing, macro_tuple)) { is_redundant = true; break; }
                }
                if (!is_redundant) {
                    std::erase_if(minimal_macro_tuples,
                            [&](const auto& e) { return is_subset(macro_tuple, e); });
                    minimal_macro_tuples.push_back(std::move(macro_tuple));
                }
            } while (next_tuple(selector, arity));

            for (const auto& macro_tuple : minimal_macro_tuples) {
                std::vector<State> s_tuple(arity);
                for (unsigned i = 0; i < arity; ++i)
                    s_tuple[i] = ctx.get_or_create_macrostate(macro_tuple[i], false);
                res_symbol_post.insert(s_tuple);
            }

            if (!res_symbol_post.empty()) {
                auto& new_s_state_post = ctx.result.delta.mutable_state_post(new_s);
                new_s_state_post.push_back(std::move(res_symbol_post));
            }
        }
    }
    return ctx.result;
} // complement_top_down

bool  is_included(const Nfta& small, const Nfta& big) {
    return false;
}


} // namespace mata::nfta
