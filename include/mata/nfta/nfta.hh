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
#include <set>
#include <utility>
#include <vector>
#include <utility>

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

    public:
        explicit Nfta(
            utils::SparseSet<State> initial_states = {},
            Alphabet* alphabet = nullptr,
            Delta delta = {}
        )
            :
              initial_states(std::move(initial_states)),
              alphabet(alphabet),
              delta(std::move(delta))
        {}
        explicit Nfta(const size_t num_of_states) : initial_states({}), alphabet(nullptr), delta(num_of_states) {}

        Nfta(const Nfta& other) = default;
        Nfta& operator=(const Nfta&) = default;

        Nfta(Nfta&&) noexcept = default;
        Nfta& operator=(Nfta&&) noexcept = default;

        /**
         * @brief Add an initial state, the state itself is also added if it didn't exist already.
         */
        void add_initial_state(const State state) {
            delta.add_state(state);
            initial_states.insert(state);
		}

        /**
         * @brief Add multiple initial states from an iterable structure.
         */
        template <typename Iterable>
        void add_initial_states(const Iterable& states) {
            add_state(*std::max_element(states.begin(), states.end()));
            initial_states.insert(states.begin(), states.end());
        }

        /**
         * @brief Add multiple final states from an initializer list.
         */
        void add_initial_states(const std::initializer_list<State> states) {
            delta.add_state(*std::ranges::max_element(states));
            initial_states.insert(states);
        }

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
        void print_readable_bottom_up(std::ostream& os = std::cout) const;

        void print_timbuk(std::ostream& os = std::cout, const std::string& name = "A") const;

        /**
         * @brief Get the set of initial states.
         */
        const utils::SparseSet<State>& get_initial_states() const { return initial_states; }

        /**
         * @brief Check if the automaton is empty - no initial states and no transitions in delta.
         */
        bool is_empty() const { return initial_states.empty() || delta.empty(); }

        /**
         * @brief Remove unused states and rename the rest.
         */
        void defragment(const BoolVector& is_staying);


        /**
         * @brief Check if the two automata are identical. Alphabets are compared as pointers.
         */
        bool operator== (const Nfta& other) const;

        /**
         * @brief Check if the two automata are identical. Alphabets are ignored.
         */
        bool has_equal_structure (const Nfta& other) const;

        /**
         * @brief Remove epsilon transitions from an automaton.
         *
         * A new automaton is build and replaces this one.
         */
        void remove_epsilon(Symbol epsilon);

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
        void swap_initial_states();


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
         * @brief Check if the automaton is complete. todo
         */
        bool is_bottom_up_complete(const utils::OrdVector<Symbol>& symbols) const;

        /**
         * @brief Check bottom-up completeness.
         *
         * Arities are disregarded.
         */
        bool is_bottom_up_complete(const utils::OrdVector<SymbolArity>& symbols_arities) const {
            return is_bottom_up_complete(collect_symbols(symbols_arities));
        }

        /**
         * @brief Check top-down completeness.
         *
         * Symbols need to exclude constants.
         */
        bool is_top_down_complete(const utils::OrdVector<Symbol>&symbols) const;

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
        void make_bottom_up_complete(const utils::OrdVector<SymbolArity>& symbols_arities, State sink = Limits::max_state);

        /**
         * @brief Complete the automaton with symbols either from its alphabet (if a ranked alphabet is used), or with all used symbols in delta.
         *
         * @param sink Sink state may be custom defined, the default value will use the next available state.
         *
         * Using default sink value is recommended, as using a higher sink value will lead to adding all states
         * before it, and all possible transitions from those states. An existing state may be used as sink, in that
         * case existing from it are NOT deleted.
         */
        void make_bottom_up_complete(State sink = Limits::max_state);

        /**
         * @brief Complete the automaton top-down with given symbols, add missing transitions leading to a sink state.
         *
         * @param symbols_arities OrdVector of symbols to be added if missing, and their arities - arity 0 symbols are ignored.
         * @param sink Sink state may be custom defined, the default value will use the next available state.
         *
         * Using default sink value is recommended, as using a higher sink value will lead to adding all states
         * before it, and all possible transitions from those states. An existing state may be used as sink, in that
         * case existing from it are NOT deleted.
         */
        void make_top_down_complete(const utils::OrdVector<std::pair<Symbol, unsigned>>& symbols_arities, State sink = Limits::max_state);

        /**
         * @brief Complete the automaton with symbols either from its alphabet (if a ranked alphabet is used), or with all used symbols in delta.
         *
         * @param sink Sink state may be custom defined, the default value will use the next available state.
         *
         * Using default sink value is recommended, as using a higher sink value will lead to adding all states
         * before it, and all possible transitions from those states. An existing state may be used as sink, in that
         * case existing from it are NOT deleted.
         */
        void make_top_down_complete(State sink = Limits::max_state);

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

        /**
         * @brief
         *
         * todo some parameter to choose an algorithm
         */
        void determinize(std::unordered_map<StateSet, State>* state_mapping = nullptr);
    }; // class Nfta

    /**
     * @brief Compute epsilon closures for each state.
     */
    std::vector<StateSet> get_epsilon_closures(const Delta& delta, Symbol epsilon, bool include_state);

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

    /** todo
    *
    */
    Nfta complement(const Nfta& aut);

    /**
    * @brief Create a product automaton. Used for union and intersection.
    */
    Nfta product(const Nfta& A, const Nfta& B, Condition cond,utils::TwoDimensionalMap<State> *state_mapping_out = nullptr);

    Nfta determinize_naive(const Nfta& aut, std::unordered_map<StateSet, State>* state_mapping = nullptr);


} // namespace mata::nfta
#endif // MATA_NFTA_H
