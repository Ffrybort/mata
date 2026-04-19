/**
 * @file nfta.hh
 * @brief Nondeterministic Finite Tree Automaton (NFTA) and its operations.
 *
 * This file contains the definition of the @c mata::nfta::Nfta class and related
 * free functions implementing operations on finite tree automata.
 *
 * @section nfta_design Design
 *
 * An NFTA is represented by the @c mata::nfta::Nfta class. States are integers
 * starting from 0. The transition relation is stored in @c mata::nfta::Delta,
 * indexed top-down: source state -> symbol -> set of target tuples. The set of
 * root states (initial for top-down and final for bottom-up
 * interpretation) is stored in a @c mata::utils::SparseSet.
 *
 * @section nfta_directions Top-down vs. Bottom-up
 *
 * Most algorithms in this library operate either top-down (starting from root
 * states, following transitions toward leaves) or bottom-up (starting from
 * constant transitions, propagating upward). The direction is noted in each
 * function's documentation. Bottom-up operations internally use a reversed
 * delta (@c mata::nfta::ReversedDelta).
 *
 * @section nfta_usage Working with NFTAs
 *
 * Some operations (product constructions, deterministic union) require the
 * automaton to be complete — every state must have a transition for every
 * non-constant symbol. Use @c make_bottom_up_complete or
 * @c make_top_down_complete to complete an automaton before passing it to
 * such operations.
 *
 * Operation outputs are not by default reduced or otherwise optimized. Apply
 * operations such as @c mata::nfta::reduce_bottom_up() or
 * @c mata::nfta::reduce_top_down() to get a reduced automaton.
 *
 * Users can create NFTAs manually by adding states and transitions using the methods provided in the @c mata::nfta::Nfta
 * and @c mata::nfta::Delta classes, or they can load an automaton from a string using the methods in the @c mata::nfta::Builder
 *
 * Any alphabet implemented in Mata can be used with NFTAs, or none at all. A ranked alphabet is implemented in the @c mata::nfta::RankedAlphabet class.
 * Some operations (like completing an automaton) require the list of all symbols and arities to use. If an unranked alphabet is used,
 * this list can either be passed to these functions, otherwise symbols in @Delta are used by default.
 */

#ifndef MATA_NFTA_H
#define MATA_NFTA_H

#include <string>
#include <iostream>
#include <utility>

#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>
#include <mata/utils/sparse-set.hh>
#include <mata/nfta/delta.hh>
#include <mata/utils/two-dimensional-map.hh>

namespace mata::nfta {

    /**
     * @brief Class representing a nondeterministic finite tree automaton.
     */
    class Nfta {
    public:
        utils::SparseSet<State> root_states; ///< A set of root states
        Alphabet* alphabet; ///< A shared alphabet (or null)
        Delta delta; ///< transition relation

        // constructors
        explicit Nfta(utils::SparseSet<State> root_states = {}, Alphabet* alphabet = nullptr, Delta delta = {})
            : root_states(std::move(root_states)), alphabet(alphabet), delta(std::move(delta)) {}
        explicit Nfta(const size_t num_of_states) : root_states({}), alphabet(nullptr), delta(num_of_states) {}

        Nfta(const Nfta& other) = default;
        Nfta& operator=(const Nfta&) = default;

        Nfta(Nfta&&) noexcept = default;
        Nfta& operator=(Nfta&&) noexcept = default;

        /**
         * @brief Add a root state, the state itself is also added if it didn't exist already.
         */
        void add_root(const State state) { // {{{
            delta.add_state(state);
            root_states.insert(state);
	    } // }}}

        /**
         * @brief Add multiple root states from an iterable structure.
         */
        template <typename Iterable>
        void add_root_states(const Iterable& states) { // {{{
            add_state(*std::max_element(states.begin(), states.end()));
            root_states.insert(states.begin(), states.end());
        } // }}}

        /**
         * @brief Add multiple final states from an initializer list.
         */
        void add_root_states(const std::initializer_list<State> states) { // {{{
            delta.add_state(*std::ranges::max_element(states));
            root_states.insert(states);
        } // }}}

        /**
         * @brief Check whether a state is root.
         */
        bool is_state_root(const State& state) const { return root_states.contains(state); }

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
         * @brief Check if the accepted language is empty.
         */
        bool is_lang_empty() const;

        /**
         * @brief Remove given states and rename the rest.
         *
         * @param is_staying[state] is false -> state gets removed.
         */
        void defragment(const BoolVector& is_staying);

        /**
         * @brief Check if the automata have identical root states and transitions. Alphabets are ignored.
         *
         * This does NOT check language equality.
         */
        bool is_identical_to(const Nfta& other) const;

        /**
         * @brief Remove epsilon transitions from an automaton.
         *
         * The automaton is modified in-place.
         *
         * @param epsilon Default or user defined epsilon symbol, it has to be unary.
         */
        void remove_epsilon_in_place(Symbol epsilon = EPSILON);

        /**
         * @brief In-place union that does not preserve determinism.
         *
         * Automata should use the same alphabet, otherwise the result automaton can be assigned either alphabet
         * and may contain symbols that are not in its alphabet.
         */
        void unite_nondet_with(const Nfta& aut);

        /**
         *@brief Swap root and non-root states. Complement a deterministic and complete automaton.
         *
         * New root states consist of all states from delta that but current root states.
         */
        void swap_root_non_root() { root_states.complement(static_cast<State>(delta.num_of_states())); }

        /**
         * @brief Complement a and deterministic automaton. Automaton gets completed.
         */
        void complement_as_deterministic(const utils::OrdVector<SymbolArity>* symbols_arities_in = nullptr);

        /**
         * @brief Check if the automaton is bottom-up deterministic.
         *
         * Every combination of symbol + set of targets appears at most once in delta. This function is expensive.
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
         * @brief Check if the automaton is complete using alphabet symbols or delta symbols (if alphabet is null).
         */
        bool is_bottom_up_complete() const;

        /**
         * @brief Check bottom-up completeness directly on a reversed delta.
         */
        bool is_bottom_up_complete(const ReversedDelta& rev_delta) const;

        /**
         * @brief Check top-down completeness.
         *
         * @param [in] symbols need to exclude constants
         */
        bool is_top_down_complete(const utils::OrdVector<Symbol>&symbols) const;

        /**
         * @brief Check top-down completeness using its alphabet (if a ranked alphabet is used) or symbols in delta.
         */
        bool is_top_down_complete() const;

        /**
         * @brief Check top-down completeness.
         *
         * @param [in] symbols_arities constant (arity 0) symbols are automatically ignored
         */
        bool is_top_down_complete(const utils::OrdVector<SymbolArity>& symbols_arities) const { //{{{
            return is_top_down_complete(collect_symbols(symbols_arities, true));
        } //}}}

        /**
         * @brief Complete the automaton bottom-up with given symbols, add missing transitions leading to a sink state.
         *
         * @param symbols_arities_in OrdVector of symbols to be added if missing, and their arities.
         * @param sink Sink state may be custom defined, the default value will use the next available state.
         *
         * Using default sink value is recommended, as using a higher sink value will lead to adding all states
         * before it, and all possible transitions from those states. An existing state may be used as sink, in that
         * case existing from it are NOT deleted.
         */
        void make_bottom_up_complete(
          const utils::OrdVector<SymbolArity> *symbols_arities_in = nullptr, State sink = Limits::max_state);

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

        /**
         * @brief Get a bool vector where vector[state] is true iff the state is top-down reachable.
         *
         * @param allowed [in, optional] filter out already unuseful states
         */
        BoolVector get_top_down_reachable(const BoolVector *allowed = nullptr) const;
        /**
         * @brief Get a bool vector where vector[state] is true iff the state is bottom-up reachable.
         *
         * @param allowed [in, optional] filter out already unuseful states
         */
        BoolVector get_bottom_up_reachable(const BoolVector *allowed = nullptr) const;

        /**
         * @brief Get a bool vector where vector[state] is true iff the state is top-down reachable.
         *
         * @param early_exit_fn [in, optional] if true, this function ends
         * @param allowed [in, optional] filter out already unuseful states
         *
         * The optional function @ early_exit_fn is applied to any newly found reachable state. If it returns true,
         * this function ends immediately, leaving any unexplored states marked as false.
         */
        template<typename OnMarked>
        BoolVector get_bottom_up_reachable_impl(OnMarked&& early_exit_fn, const BoolVector *allowed = nullptr) const;

        /**
         * @brief Remove top-down unreachable states
         */
        void reduce_top_down();

        /**
         * @brief Remove bottom-up unreachable states
         */
        void reduce_bottom_up();

        /**
         * @brief Reduce top-down, then bottom-up, then top-down again.
         */
        void reduce_top_bottom_top();
        /**
         * @brief Reduce bottom-up, then top-down.
         */
        void reduce_bottom_top();

    }; // class Nfta

    /**
     * @brief Remove epsilon transitions from an automaton.
     *
     * @param aut Input automaton
     * @param epsilon Symbol to consider as epsilon
     * @throws std::runtime_error if epsilon is not unary (arity 1)
     */
    Nfta remove_epsilon(const Nfta& aut, Symbol epsilon);

    /**
     * @brief Union of two automata not preserving determinism.
     *
     * @param A, B [in] Automata to unite
     *
     * Automata should use the same alphabet, otherwise the result automaton can be assigned either alphabet and may
     * contain symbols that are not in its alphabet.
     */
    Nfta union_nondet(const Nfta& A, const Nfta& B);

    /**
     * @brief Union preserving bottom-up determinism, computed by product construction.
     *
     * @param A, B [in] Automata to unite, both must be bottom-up complete
     * @param state_mapping_out [out, optional] Mapping state pairs -> product state
     *
     * This implementation is slow. The result is bottom-up reduced, but not top-down reduced.
     */
    Nfta union_det_on_complete(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out = nullptr);

    /**
    * @brief Union preserving bottom-up determinism, computed by product construction.
    *
    * @param A, B [in] Automata to unite, both must be bottom-up complete
    * @param state_mapping_out [out, optional] Mapping state pairs -> product state
    *
    * This implementation is slow. The result is bottom-up reduced, but not top-down reduced.
    */
    Nfta union_det(Nfta& A, Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out = nullptr);

    /**
     * @brief Complement the automaton using (optimized) determinization and swapping final and non-final states.
     *
     * @param aut [in] Input automaton to complement.
     * @param symbols_arities_in [in, optional] Vector of (symbol, arity) pairs to consider instead of alphabet/used symbols.
     * @return Complement automaton.
     *
     * If @ symbols_arities are not provided, the function defaults to alphabet symbols (if a ranked alphabet is used)
     * or to used symbols in delta. Result is bottom-up deterministic, complete and reduced.
     */
    Nfta complement_classical(const Nfta& aut, const utils::OrdVector<SymbolArity>* symbols_arities_in = nullptr);

    /**
     * @brief Create a product automaton.
     *
     * @param A, B Automata to intersect.
     * @param state_mapping_out [out, optional] Mapping state pairs -> product state.
     *
     * Both automata must be complete over the same set of symbols. The result automaton is constructed directly
     * top-down. Result is top-down reduced, but not bottom-up reduced.
     */
    Nfta intersection(const Nfta& A, const Nfta& B, utils::TwoDimensionalMap<State> *state_mapping_out = nullptr);

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
     * or to used symbols in delta. Result is top-down reduced.
     */
    Nfta complement_top_down(
      const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping = nullptr,
      const utils::OrdVector<SymbolArity>* symbols_arities_in = nullptr
    );

    enum class ComplementMethod { Classical, TopDown };
    /**
     * @brief Check if the language recognized by @p small in a subset of the language recognized by @p big.
     *
     * @param smaller, bigger input automata
     * @param method complementation method to use (classical or top-down)
     * @return true if L(small) <= L(big), false otherwise
     */
    bool is_lang_included(const Nfta& smaller, const Nfta& bigger, ComplementMethod method = ComplementMethod::Classical);

    /**
     * @brief Check if the language recognized by @p A equal to the language recognized by @p B.
     *
     * @param A, B input automata
     * @param method complementation method to use (classical or top-down)
     * @return true if L(A) == L(B), false otherwise
     */
    bool is_lang_equal(const Nfta& A, const Nfta& B, ComplementMethod method = ComplementMethod::Classical);

} // namespace mata::nfta
#endif // MATA_NFTA_H
