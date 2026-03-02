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

namespace mata::nfta {
    class Nfta {
    public:
        utils::SparseSet<State> final_states; // a set of final (or initial) states
        Alphabet* alphabet;
        Delta delta; // states live in delta, so do functions like add_state()

    public:
        explicit Nfta(
            utils::SparseSet<State> final_states = {},
            Alphabet* alphabet = nullptr,
            Delta delta = {}
        )
            :
              final_states(std::move(final_states)),
              alphabet(alphabet),
              delta(std::move(delta))
        {}
        explicit Nfta(const size_t num_of_states) : final_states({}), alphabet(nullptr), delta(num_of_states) {}

        Nfta(const Nfta& other) = default;
        Nfta& operator=(const Nfta&) = default;

        Nfta(Nfta&&) noexcept = default;
        Nfta& operator=(Nfta&&) noexcept = default;

        /**
         * @brief Add a final state, the state itself is also added if it didn't exist already.
         */
        void add_final_state(const State state) {
            delta.add_state(state);
            final_states.insert(state);
		}

        /**
         * @brief Add multiple final states from an iterable structure.
         */
        template <typename Iterable>
        void add_final_states(const Iterable& states)
        {
            add_state(*std::max_element(states.begin(), states.end()));
            final_states.insert(states.begin(), states.end());
        }

        /**
         * @brief Add multiple final states from an initializer list.
         */
        void add_final_states(const std::initializer_list<State> states)
        {
            delta.add_state(*std::ranges::max_element(states));
            final_states.insert(states);
        }

        /**
         * @brief Check whether a state is final (or initial).
         */
        bool is_state_final(const State& state) const { return final_states.contains(state); }

        /**
         * @brief Print the automaton in a parsable mata format.
         */
        void print_mata(std::ostream& os) const;

        /**
         * @brief Print the automaton in an easy-to-read format.
         */
        void print_readable(std::ostream& os, const std::string& type = "") const;

        /**
         * @brief Get the set of final states.
         */
        const utils::SparseSet<State>& get_final_states() const { return final_states; }

        /**
         * @brief Remove unused states and rename the rest.
         */
        void defragment(const BoolVector& is_staying);


        // comparing delta and final states, ignoring alphabet
        bool operator== (const Nfta& other) const;

        /**
         * @brief Remove epsilon transitions from an automaton. todo validate
         *
         * A new automaton is build and replaces this one.
         */
        void remove_epsilon(Symbol epsilon);

        /**
         * @brief Remove epsilon transitions from an automaton. todo
         *
         * The automaton is modified in-place.
         */
        void remove_epsilon_in_place(Symbol epsilon);

        /**
         * @brief In-place union. Does not preserve determinism. todo
         */
        void union_nondet_in_place(const Nfta& aut);

        /**
         * @brief Check if the automaton is deterministic. todo
         */
        bool is_deterministic() const { return true; }

        /**
         * @brief Check if the automaton is complete. todo
         */
        bool is_complete() const { return true; }

    }; // class Nfta

    /**
     * @brief Compute epsilon closures for each state. todo move to delta?
     */
    std::vector<StateSet> get_epsilon_closures(const Delta& delta, Symbol epsilon, bool include_state);

    /**
     * @brief Union of two automata not preserving determinism. todo
     */
    Nfta union_nondet(const Nfta& A, const Nfta& B);

    /**
     * @brief Union preserving determinism, computed by product construction. todo
     */
    Nfta union_product(const Nfta& A, const Nfta& B);

    /**
     * @brief Intersection. todo
     */
    Nfta intersection(const Nfta& A, const Nfta& B);

} // namespace mata::nfta
#endif // MATA_NFTA_H
