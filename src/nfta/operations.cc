/**
 * @file operations.cc
 *
 * @brief Implementation of NFTA operations.
 */

#include "mata/nfta/nfta.hh" // todo resolve headers
#include "mata/utils/two-dimensional-map.hh"
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
const utils::OrdVector<SymbolArity>& resolve_symbols_arities (
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



std::vector<StateSet> Delta::get_epsilon_closures(const Symbol epsilon) const {
    const size_t num_states = num_of_states();
    std::vector<StateSet> result { num_states };
    std::vector<StateSet> epsilon_successors { num_states };

    // adding immediate successors
    for (State i = 0; i < static_cast<State>(num_states); i++) {
        epsilon_successors[i] = state_posts_[i].get_successors(epsilon);
        result[i] = epsilon_successors[i];
        result[i].insert(i);
    }

    // get closure
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < num_states; ++i) {
            StateSet& src_eps_cl = result[i];
            for (const State tgt : result[i]) {
                const StateSet& tgt_eps_cl = result[tgt];
                changed = changed || src_eps_cl.insert(tgt_eps_cl);
            }
        }
    }
    return result;
} // get_epsilon_closures

Nfta remove_epsilon(const Nfta& aut, const Symbol epsilon) {
    const auto num_of_states = static_cast<State>(aut.delta.num_of_states());
    std::vector<StateSet> epsilon_closures = aut.delta.get_epsilon_closures(epsilon);

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

struct ProductContext {
    Nfta result;
    std::deque<State> worklist{}; // Set of product states to process.
    std::vector<State> processed;
    bool changed;
    utils::TwoDimensionalMap<State> *state_mapping;
    std::optional<utils::TwoDimensionalMap<State>> local_mapping;

    explicit ProductContext(const size_t num_states_A, const size_t num_states_B,
        utils::TwoDimensionalMap<State>* state_mapping_out)
        : result(), worklist(), processed(), changed(false), state_mapping(), local_mapping(std::nullopt) {
        if (state_mapping_out) {
            state_mapping = state_mapping_out;
        }
        else {
            local_mapping.emplace(num_states_A, num_states_B);
            state_mapping = &local_mapping.value();
        }
        processed.reserve(num_states_A * num_states_B);
    }

    // get or create product state, push a new state into worklist
    State get_product_state (const State state_A, const State state_B) {
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

    // initialize with pairs of initial states
    void initialize_top_down(const Nfta& A, const Nfta& B) {
        // Initialize worklist with initial state pairs
        for (const State initial_A : A.root_states) {
            for (const State initial_B : B.root_states) {
                // Update product with initial state pairs.
                const State product_initial_state = get_product_state(initial_A, initial_B);
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
            assert(symbol_A == const_it_B->first && "mata::nfta Automata contain different constant symbols.");

            for (const State state_A : const_it_A->second) {
                for (const State state_B : const_it_B->second) {
                    const State product_state = get_product_state(state_A, state_B);
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

#include <iostream>

template<typename T>
void print_vec(const std::vector<T>& v) {
    std::cout << "[ ";
    for (const auto& x : v) std::cout << x << " ";
    std::cout << "]";
}

Nfta union_det_on_complete(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State>* state_mapping_out) {
    using SymbolCache  =
        // [det_state][position] -> vector of targets sets
        std::vector< // state
            std::vector< // position
                utils::OrdVector<const ReversedDelta::RevStateTuplePost*> // data
            >
        >;

    if (A.root_states.empty() && B.root_states.empty()) { return create_empty(); }

    const ReversedDelta rev_delta_A = A.delta.get_reversed();
    const ReversedDelta rev_delta_B = B.delta.get_reversed();
    assert(A.is_complete(rev_delta_A) && B.is_complete(rev_delta_B) &&
        "mata::nfta::union_det_on_complete automata must be complete");
    assert(rev_delta_A.symbol_posts.size() == rev_delta_B.symbol_posts.size() &&
        "union_det: automata must have the same number of symbols");

    std::unordered_map<Symbol, SymbolCache> cache_A;
    std::unordered_map<Symbol, SymbolCache> cache_B;

    ProductContext ctx(A.delta.num_of_states(), B.delta.num_of_states(), state_mapping_out);
    ctx.initialize_bottom_up(rev_delta_A, rev_delta_B, A, B);

    auto fill_cache = [&](
        std::unordered_map<Symbol, SymbolCache> &cache, const ReversedDelta &rev_delta, const size_t num_of_states) {
        cache.reserve(rev_delta.symbol_posts.size());

        std::vector<std::vector<utils::OrdVector<const ReversedDelta::RevStateTuplePost*>>> collector;
        collector.resize(num_of_states);

        for (const auto& sym_tr : rev_delta.symbol_posts) {
            Symbol symbol = sym_tr.symbol;
            const unsigned arity = sym_tr.get_arity();
            for (auto& r : collector) { r.resize(arity); }

            auto& symbol_cache = cache[symbol];
            symbol_cache.resize(num_of_states);
            for (unsigned i = 0; i < num_of_states; ++i) { symbol_cache[i].resize(arity); }

            for (auto& src_tr : sym_tr.state_tuple_posts) {
                for (unsigned i = 0; i < arity; ++i) {
                    collector[src_tr.sources[i]][i].push_back(&src_tr);
                }
            }

            for (unsigned i = 0; i < num_of_states; ++i) {
                for (unsigned j = 0; j < arity; ++j) {
                    symbol_cache[i][j] = utils::OrdVector(std::move(collector[i][j]));
                    collector[i][j].clear();
                }
            }
        }
    };

    fill_cache(cache_A, rev_delta_A, A.delta.num_of_states());
    fill_cache(cache_B, rev_delta_B, B.delta.num_of_states());

    // returns false if any child pair is unknown, otherwise fills source_tuple
    auto try_build_source_tuple = [&](const ReversedDelta::RevStateTuplePost& src_tr_A,
        const ReversedDelta::RevStateTuplePost& src_tr_B, unsigned arity, std::vector<State>& source_tuple) -> bool {
        assert(src_tr_B.sources.size() == arity);
        for (size_t i = 0; i < arity; ++i) {
            const State ps = ctx.state_mapping->get(src_tr_A.sources[i], src_tr_B.sources[i]);
            if (ps == Limits::max_state) { return false; }
            source_tuple[i] = ps;
        }
        return true;
    };

    auto process_target_pair = [&](const Symbol symbol, const std::vector<State>& source_tuple,
        const State target_A, const State target_B) {
        const State product_target = ctx.get_product_state(target_A, target_B);
        if (A.root_states.contains(target_A) || B.root_states.contains(target_B)) {
            ctx.result.add_root(product_target);
        }
        ctx.result.delta.add(product_target, symbol, source_tuple);
    };

    while (!ctx.worklist.empty()) {
        State new_prod = ctx.worklist.back();
        ctx.worklist.pop_back();

        State state_A = ctx.state_mapping->get_first_inverted(new_prod);
        State state_B = ctx.state_mapping->get_second_inverted(new_prod);

        auto sym_it_A = rev_delta_A.symbol_posts.begin();
        auto sym_it_B = rev_delta_B.symbol_posts.begin();
        while (sym_it_A != rev_delta_A.symbol_posts.end()) {

            if (sym_it_A->is_constant()) {
                ++sym_it_A;
                ++sym_it_B;
                continue;
            }

            Symbol symbol = sym_it_A->symbol;
            unsigned arity = sym_it_A->get_arity();

            assert(sym_it_B != rev_delta_B.symbol_posts.end());
            assert(sym_it_B->get_arity() == arity);
            assert(sym_it_B->symbol == symbol);

            auto& state_A_cache = cache_A[symbol][state_A];
            auto& state_B_cache = cache_B[symbol][state_B];

            std::vector<State> source_tuple(arity);

            for (unsigned i = 0; i < arity; ++i) {
                for (auto src_tr_A : state_A_cache[i]) {
                    for (auto src_tr_B : state_B_cache[i]) {
                        if (!try_build_source_tuple(*src_tr_A, *src_tr_B, arity, source_tuple)) continue;
                        for (const State target_A : src_tr_A->targets) {
                            for (const State target_B : src_tr_B->targets) {
                                process_target_pair(symbol, source_tuple, target_A, target_B);
                            }
                        }
                    }
                }
            }
            ++sym_it_A;
            ++sym_it_B;
        }
    }

    ctx.result.alphabet = A.alphabet;
    return std::move(ctx.result);
}

Nfta union_det(Nfta& A, Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out) {
    if (A.alphabet == B.alphabet && dynamic_cast<RankedAlphabet*>(A.alphabet)) {
        A.make_complete();
        B.make_complete();
    }
    else {
        auto symbols = A.delta.get_used_symbols_arities();
        symbols.insert(B.delta.get_used_symbols_arities());
        A.make_complete(&symbols);
        B.make_complete(&symbols);
    }
    return union_det_on_complete(A, B, state_mapping_out);
}

Nfta intersection(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out) {
    if (A.root_states.empty() || B.root_states.empty()) { return create_empty(); }

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
                        State product_target = ctx.get_product_state(targets_A[i], targets_B[i]);
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
    return std::move(ctx.result);
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
    if (root_states.size() > 1) { return false; }
    if (delta.empty()) { return true; }

    const auto num_of_states = static_cast<State>(delta.num_of_states());
    for (State i = 0; i < num_of_states; ++i) {
        for (const auto& symbol_post : delta[i]) { if (symbol_post.target_tuples.size() != 1) { return false; } }
    }
    return true;
} // is_top_down_deterministic

bool Nfta::is_complete() const {
    if (alphabet) {
        return is_complete(alphabet->get_alphabet_symbols());
    }
    return is_complete(delta.get_used_symbols());
}

bool Nfta::is_complete(const utils::OrdVector<Symbol> &symbols) const {
    const size_t num_of_states = delta.num_of_states();
    ReversedDelta rev_delta = delta.get_reversed();
    if (rev_delta.symbol_posts.size() < symbols.size()) {
        return false;
    }
    for (const auto& symbol_tr : rev_delta.symbol_posts) {
        if (!symbols.contains(symbol_tr.symbol)) {
            unknown_symbol_in_delta(alphabet->try_reverse_translate_symbol(symbol_tr.symbol));
        }
        assert(!symbol_tr.state_tuple_posts.empty() && "Source transitions not empty");
        const size_t arity = symbol_tr.get_arity();
        if (symbol_tr.state_tuple_posts.size() != ipow(num_of_states, arity)) { return false; }

        #ifndef NDEBUG
        for (const auto& source_tr : symbol_tr.state_tuple_posts) {
            assert(source_tr.sources.size() == arity);
        }
        #endif
    } // for symbol transitions
    return true;
} // is_complete

bool Nfta::is_complete(const ReversedDelta& rev_delta) const {
    const size_t num_of_states = delta.num_of_states();
    for (const auto& symbol_tr : rev_delta.symbol_posts) {
        assert(!symbol_tr.state_tuple_posts.empty() && "Source transitions not empty");
        const size_t arity = symbol_tr.get_arity();
        if (symbol_tr.state_tuple_posts.size() != ipow(num_of_states, arity)) { return false; }

        #ifndef NDEBUG
        for (const auto& source_tr : symbol_tr.state_tuple_posts) {
            assert(source_tr.sources.size() == arity);
        }
        #endif
    } // for symbol transitions
    return true;
} // is_complete

void Nfta::make_complete(const utils::OrdVector<SymbolArity> *symbols_arities_in, State sink) {
    utils::OrdVector<SymbolArity> tmp;
    const utils::OrdVector<SymbolArity> &symbols_arities = resolve_symbols_arities(*this, symbols_arities_in, tmp);

    auto rev_delta = delta.get_reversed();

    if (sink == Limits::max_state) { sink = delta.add_state(); }
    std::cout << "sink: " << sink << std::endl;
    delta.resize_for_states(sink);
    const size_t num_of_states = delta.num_of_states();

    StatePost& sink_state_post = delta.mutable_state_post(sink);
    const bool sink_sp_empty = sink_state_post.empty();

    const bool delta_empty = delta.empty();

    auto rev_delta_it = rev_delta.symbol_posts.begin();
    const auto rev_delta_end = rev_delta.symbol_posts.end();

    auto input_symbols_it = symbols_arities.begin();
    const auto input_symbols_end = symbols_arities.end();

    size_t transition_count = 0;

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

        static const decltype(rev_delta_it->state_tuple_posts) empty{};
        auto source_tr_it  = empty.end();
        auto source_tr_end = empty.end();

        if (delta_empty || rev_delta_it == rev_delta_end || symbol < rev_delta_it->symbol) {
            ++input_symbols_it;
            // add all
        } else  { // symbols match
            assert(symbol == rev_delta_it->symbol && "Symbols do not match when they should");
            assert(arity == rev_delta_it->state_tuple_posts.at(0).sources.size() && "Wrong arity");

            source_tr_it  = rev_delta_it->state_tuple_posts.cbegin();
            source_tr_end = rev_delta_it->state_tuple_posts.cend();

            const auto old_rev_delta_it = rev_delta_it;
            ++rev_delta_it;
            ++input_symbols_it;

            // if there are already all combinations, continue
            const size_t expected_num_of_transitions = ipow(num_of_states, arity);
            assert(old_rev_delta_it->state_tuple_posts.size() <= expected_num_of_transitions);
            if (old_rev_delta_it->state_tuple_posts.size() == expected_num_of_transitions) { continue; }
        } // else
        std::vector<State> tuple(arity, 0);
        SymbolPost new_symbol_post {symbol};
        do {
            if (!delta_empty && source_tr_it != source_tr_end && source_tr_it->sources == tuple) {
                ++source_tr_it; // tuple is already present
            } else {
                // we are working with symbols in order, so push back is fine
                new_symbol_post.push_back(tuple);
                ++transition_count;
                if (transition_count % 10000 == 0) {
                    std::cerr << "Transitions so far: " << transition_count << "\n";
                }
            }
        } while (next_tuple(tuple, num_of_states));
        if (sink_sp_empty) { sink_state_post.push_back(std::move(new_symbol_post)); }
        else { delta.add(sink, new_symbol_post); }
    } // while rev_delta_it != end OR input_symbols_it != end

    #ifndef NDEBUG
    assert(delta.is_sorted());
    const utils::OrdVector<Symbol> symbols = collect_symbols(symbols_arities);
    // assert(is_complete(symbols));
    #endif
} // make_complete

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
    using SymbolCache =
    std::vector<
        std::vector<
            utils::OrdVector<const ReversedDelta::RevStateTuplePost*>
        >
    >;

    const ReversedDelta rev_delta = delta.get_reversed(allowed);
    const size_t num_of_states = delta.num_of_states();
    BoolVector marked(num_of_states, false);
    std::unordered_map<Symbol, SymbolCache> cache;

    cache.reserve(rev_delta.symbol_posts.size());
    std::vector<std::vector<utils::OrdVector<const ReversedDelta::RevStateTuplePost*>>> collector;
    collector.resize(num_of_states);

    for (const auto& sym_tr : rev_delta.symbol_posts) {
        const Symbol symbol = sym_tr.symbol;
        const unsigned arity = sym_tr.get_arity();
        for (auto& r : collector) { r.resize(arity); }

        auto& symbol_cache = cache[symbol];
        symbol_cache.resize(num_of_states);
        for (unsigned i = 0; i < num_of_states; ++i) { symbol_cache[i].resize(arity); }

        for (const auto& src_tr : sym_tr.state_tuple_posts) {
            for (unsigned i = 0; i < arity; ++i) {
                collector[src_tr.sources[i]][i].push_back(&src_tr);
            }
        }

        for (unsigned i = 0; i < num_of_states; ++i) {
            for (unsigned j = 0; j < arity; ++j) {
                symbol_cache[i][j] = utils::OrdVector(std::move(collector[i][j]));
                collector[i][j].clear();
            }
        }
    }

    std::deque<State> worklist;

    auto mark = [&](const State state) {
        if (!marked[state]) { worklist.push_back(state); }
        marked[state] = true;
    };

    for (const State state : rev_delta.get_initial_states()) {
        mark(state);
    }

    while (!worklist.empty()) {
        const State current = worklist.front();
        worklist.pop_front();

        for (auto& symbol_cache : cache | std::views::values) {
            if (current >= symbol_cache.size()) { continue; }
            for (unsigned pos = 0; pos < symbol_cache[current].size(); ++pos) { // todo cach by pos has no benefit
                for (const auto* src_tr : symbol_cache[current][pos]) {
                    bool all_marked = true;
                    for (const State s : src_tr->sources) {
                        if (!marked[s]) { all_marked = false; break; }
                    }
                    if (!all_marked) { continue; }
                    for (const State target : src_tr->targets) {
                        if (!marked[target]) {
                            mark(target);
                            if (early_exit_fn(target, marked)) { return marked; }
                        }
                    }
                }
            }
        }
    }
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

void Nfta::remove_top_down_unreachable() {
    const BoolVector marked = get_top_down_reachable();
    defragment(marked);
}

void Nfta::remove_bottom_up_unreachable() {
    const BoolVector marked = get_bottom_up_reachable();
    defragment(marked);
}

void Nfta::remove_unreachable_top_bottom_top() {
    BoolVector marked = get_top_down_reachable();
    marked = get_bottom_up_reachable(&marked);
    marked = get_top_down_reachable(&marked);
    defragment(marked);
}

void Nfta::remove_unreachable_bottom_top() {
    BoolVector marked = get_bottom_up_reachable();
    marked = get_top_down_reachable(&marked);
    defragment(marked);
}


enum class ComplementMethod;
bool is_lang_included(const Nfta& smaller, const Nfta& bigger, const ComplementMethod method) {
    Nfta bigger_compl{};
    if (method == ComplementMethod::Classical) { bigger_compl = complement(bigger); } // todo params
    else if (method == ComplementMethod::TopDown) { bigger_compl = complement_top_down(bigger); }
    else { throw std::runtime_error("Complement method not implemented"); }
    const Nfta inter = intersection(smaller, bigger_compl);
    return inter.is_lang_empty();
}

bool is_lang_included_opt(const Nfta& smaller, const Nfta& bigger) {
    // MacrostateContext ctx(bigger, nullptr);
    // const ReversedDelta bigger_rev_delta = bigger.delta.get_reversed();
    //
    // struct Item {
    //     State smaller;
    //     State bigger_s;
    //     StateSet bigger_macro;
    // };
    // struct Compare {
    //     bool operator()(const Item& a, const Item& b) const {
    //         return a.bigger_macro.size() > b.bigger_macro.size(); // sorted by state size
    //     }
    // };
    // std::priority_queue<Item, std::vector<Item>, Compare> worklist;
    // std::vector<Item> processed;
    //
    // auto get_or_create = [&](const State smaller, const StateSet& bigger_states) -> State {
    //     bool is_new = !ctx.mapping->contains(bigger_states);
    //     State s = ctx.get_or_create_macrostate(bigger_states);
    //     if (is_new) {
    //         worklist.push(Item{smaller, s, bigger_states});
    //         if (aut.root_states.intersects_with(bigger_states)) { ctx.result.add_root(s); }
    //     }
    //     return s;
    // };
    //
    // // initialize with constant transitions
    // for (auto [symbol, initial_states] : rev_delta.get_initial_states_by_symbol()) {
    //     const State new_s = get_or_create(initial_states);
    //     ctx.result.delta.add(new_s, symbol, {});
    // }
    //
    // return false;
}

bool is_lang_equal(const Nfta& A, const Nfta& B, const ComplementMethod method) {
    return is_lang_included(A, B, method) && is_lang_included(B, A, method);
}

} // namespace mata::nfta
