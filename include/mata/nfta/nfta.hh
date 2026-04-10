/**
 * @file nfta.hh
 * @brief Implementation of a nondeterministic finite tree automaton.
 *
 *
 */

#ifndef MATA_NFTA_H
#define MATA_NFTA_H

#include <string>
#include <iostream>
#include <utility>
#include <vector>

#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>
#include <mata/utils/sparse-set.hh>
#include <mata/nfta/delta.hh>
#include <mata/utils/two-dimensional-map.hh>

namespace mata::nfta {
    class Nfta {
    public:
        utils::SparseSet<State> initial_states; // a set of initial/final states
        Alphabet* alphabet;
        Delta delta; // states live in delta, so do functions like add_state()

        explicit Nfta(utils::SparseSet<State> initial_states = {}, Alphabet* alphabet = nullptr, Delta delta = {})
            : initial_states(std::move(initial_states)), alphabet(alphabet), delta(std::move(delta)) {}
        explicit Nfta(const size_t num_of_states) : initial_states({}), alphabet(nullptr), delta(num_of_states) {}

        Nfta(const Nfta& other) = default;
        Nfta& operator=(const Nfta&) = default;

        Nfta(Nfta&&) noexcept = default;
        Nfta& operator=(Nfta&&) noexcept = default;

        /**
         * @brief Add an initial state, the state itself is also added if it didn't exist already.
         */
        void add_initial_state(const State state) { // {{{
            delta.add_state(state);
            initial_states.insert(state);
	} // }}}

        /**
         * @brief Add multiple initial states from an iterable structure.
         */
        template <typename Iterable>
        void add_initial_states(const Iterable& states) { // {{{
            add_state(*std::max_element(states.begin(), states.end()));
            initial_states.insert(states.begin(), states.end());
        } // }}}

        /**
         * @brief Add multiple final states from an initializer list.
         */
        void add_initial_states(const std::initializer_list<State> states) { // {{{
            delta.add_state(*std::ranges::max_element(states));
            initial_states.insert(states);
        } // }}}

        /**
         * @brief Check whether a state is initial.
         */
        bool is_state_initial(const State& state) const { return initial_states.contains(state); }

        /**
         * @brief Print the automaton in a parsable mata format.
         */
        void print_mata(std::ostream& os = std::cout) const;

        /**
         * @brief Print the automaton in an easy-to-read format.
         */
        void print_readable(std::ostream& os = std::cout) const;

        /**
         * @brief Print the automaton is a bottom up format.
         */
        void print_readable_bottom_up(std::ostream& os = std::cout) const;

        /**
         * @brief Print the automaton is timbuk parsable format.
         */
        void print_timbuk(std::ostream& os = std::cout, const std::string& name = "A") const;

        /**
         * @brief Get the set of initial states.
         */
        const utils::SparseSet<State>& get_initial_states() const { return initial_states; }

        /**
         * @brief Check if the accepted language is empty.
         */
        bool is_lang_empty() const;

        /**
         * @brief Remove given states and rename the rest.
         */
        void defragment(const BoolVector& is_staying);

        /**
         * @brief Check if the automata have identical initial states and transitions. Alphabets are ignored.
         *
         * This does NOT check language equality.
         */
        bool is_identical_to(const Nfta& other) const;

        /**
         * @brief Remove epsilon transitions from an automaton.
         *
         * The automaton is modified in-place.
         */
        void remove_epsilon_in_place(Symbol epsilon);

        /**
         * @brief In-place union. Does not preserve determinism.
         */
        void unite_nondet_with(const Nfta& aut);

        /**
         *@brief Swap initial and non-initial states.
         *
         * New initial states consist of all states from delta that but current initial states.
         */
        void swap_initial_states() { initial_states.complement(static_cast<State>(delta.num_of_states())); }

        /**
         * @brief Complement an already and deterministic automaton.
         */
        void complement_as_deterministic();

        /**
         * @brief Check if the automaton is bottom-up deterministic.
         *
         * Every combination of symbol + set of targets appears at most once in delta.
         * This function is expensive.
         */
        bool is_bottom_up_deterministic() const;

        /**
         * @brief Check if the automaton is top-down deterministic.
         *
         * For every source and symbol there is at most one set of targets.
         */
        bool is_top_down_deterministic() const;

        /**
         * @brief Check if the automaton is complete.
         */
        bool is_bottom_up_complete(const utils::OrdVector<Symbol> &symbols) const;
        /**
         * @brief Check if the automaton is complete.
         *  todo test
         */
        bool is_bottom_up_complete() const;

        /**
         * @brief Check bottom-up completeness directly on a reversed delta.
         */
        bool is_bottom_up_complete(const ReversedDelta& rev_delta) const;

        /**
         * @brief Check top-down completeness.
         *
         * Symbols need to exclude constants.
         */
        bool is_top_down_complete(const utils::OrdVector<Symbol>&symbols) const;

        /**
         * @brief Check top-down completeness using its alphabet (if a ranked alphabet is used) or symbols in delta.
         */
        bool is_top_down_complete() const;

        /**
         * @brief Check top-down completeness.
         *
         * Constant (arity 0) symbols are automatically ignored.
         */
        bool is_top_down_complete(const utils::OrdVector<SymbolArity>& symbols_arities) const {
            return is_top_down_complete(collect_symbols(symbols_arities, true));
        }

        /**
         * @brief Complete the automaton bottom-up with given symbols, add missing transitions leading to a sink state.
         *
         * @param symbols_arities OrdVector of symbols to be added if missing, and their arities.
         * @param sink Sink state may be custom defined, the default value will use the next available state.
         *
         * Using default sink value is recommended, as using a higher sink value will lead to adding all states
         * before it, and all possible transitions from those states. An existing state may be used as sink, in that
         * case existing from it are NOT deleted.
         */
        void make_bottom_up_complete(const utils::OrdVector<SymbolArity> *symbols_arities_in = nullptr, State sink = Limits::max_state);

        /**
         * @brief Complete the automaton top-down with given symbols, add missing transitions leading to a sink state.
         *
         * @param sink [in, optional] Sink state may be custom defined or default value will use the next available state.
         * @param symbols_arities_in [in, optional] Symbols and arities to consider instead of alphabet/used symbols.
         * @return Complement automaton.
         *
         * If @ symbols_arities are not provided, the function defaults to alphabet symbols (if a ranked alphabet is used)
         * or to used symbols in delta.
         *
         * Using default sink value or an existing state is recommended, as using a higher sink value will lead to
         * adding all states before it, and all possible transitions from those states. If an existing state is used,
         * existing transitions from it are deleted.
         */
        void make_top_down_complete(const utils::OrdVector<SymbolArity> *symbols_arities_in = nullptr,
          State sink = Limits::max_state);


        BoolVector get_top_down_reachable() const;
        BoolVector get_bottom_up_reachable() const;

        /**
         * @brief Remove top-down unreachable states
         */
        void reduce_top_down();

        /**
         * @brief Remove bottom-up unreachable states
         */
        void reduce_bottom_up_down();
    }; // class Nfta

    /**
     * @brief Compute epsilon closures for each state. todo move to delta?
     */
    std::vector<StateSet> get_epsilon_closures(const Delta& delta, Symbol epsilon, bool include_state);

    /**
     * @brief Remove epsilon transitions from an automaton.
     */
    Nfta remove_epsilon(const Nfta& aut, Symbol epsilon);

    /**
     * @brief Union of two automata not preserving determinism.
     */
    Nfta union_nondet(const Nfta& A, const Nfta& B);

    /**
     * @brief Union preserving determinism, computed by product construction.
     *
     * Both input automata must be epsilon free.
     */
    Nfta union_product(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out = nullptr);

    /**
     * @brief Intersection.
     *
     * Both input automata must be epsilon free.
     */
    Nfta intersection(const Nfta& A, const Nfta& B);

    /**
     * @brief Complement the automaton using (optimized) determinization and swapping final and non-final states.
     *
     * @param aut [in] Input automaton to complement.
     * @param symbols_arities [in, optional] Optional vector of (symbol, arity) pairs to consider instead of alphabet/used symbols.
     * @return Complement automaton.
     *
     * If @ symbols_arities are not provided, the function defaults to alphabet symbols (if a ranked alphabet is used)
     * or to used symbols in delta.
     */
    Nfta complement_classical(const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities = nullptr);

    /**
     * @brief Create a product automaton. Used for union and intersection.
     */
    Nfta product(const Nfta& A, const Nfta& B, Condition cond,utils::TwoDimensionalMap<State> *state_mapping_out = nullptr);

    /**
     * @brief Determinize an automaton.
     *
     * Only optimization is that only (bottom-up) reachable states are constructed.
     */
    Nfta determinize_naive(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping = nullptr);

    /**
     * @brief Determinize an automaton.
     *
     * Only (bottom-up) reachable states are constructed. todo describe
     */
    Nfta determinize_optimized(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping = nullptr);

    /**
     * @brief Construct a complement automaton without determinizing, directly top down.
     *
     * @param aut [in] Input automaton to complement.
     * @param state_mapping [out, optional] Optional mapping macrostate (set of states) -> result state.
     * @param symbols_arities_in [in, optional] Optional vector of (symbol, arity) pairs to consider instead of alphabet/used symbols.
     * @return Complement automaton.
     *
     * If @ symbols_arities are not provided, the function defaults to alphabet symbols (if a ranked alphabet is used)
     * or to used symbols in delta.
     */
    Nfta complement_top_down(
      const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping = nullptr,
      const utils::OrdVector<SymbolArity>* symbols_arities_in = nullptr
    );

    bool  is_included(const Nfta& small, const Nfta& big, Alphabet* alphabet = nullptr);

} // namespace mata::nfta
#endif // MATA_NFTA_H
