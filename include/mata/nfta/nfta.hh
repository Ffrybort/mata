/**
 * @file nfta.hh
 * @brief Implementation of a nondeterministic finite tree automaton.
 *
 * The automaton can be either top-down or bottom-up, given by its type.
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

namespace mata::nfta
{
    class Nfta
    {
    public:
        AutType type;
        unsigned num_of_states;
        utils::SparseSet<State> root_states; // initial or final states
        Alphabet* alphabet;
        ArityMap arities;
        Delta delta;

    public:
        explicit Nfta(
            AutType type = None,
            const unsigned num_of_states = 0,
            const utils::SparseSet<State>& root_states = {},
            const Delta& delta = {},
            Alphabet* alphabet = nullptr,
            ArityMap arities = {}
        )
            : type(type),
              num_of_states(num_of_states),
              root_states(root_states),
              alphabet(alphabet),
              arities(std::move(arities)),
              delta(delta)
        {
        }
        Nfta(const Nfta&) = delete; // todo implement moving
        Nfta& operator=(const Nfta&) = delete;

        Nfta(Nfta&&) noexcept = default;
        Nfta& operator=(Nfta&&) noexcept = default;

        /**
         * @brief Add a state to the automaton.
         */
        State add_state() { num_of_states++; return num_of_states - 1;  }

        /**
         * @brief Add a state to root states. Ignores duplicates.
         *
         */
        void add_root_state(const State& state)
        {
            root_states.insert(state);
        }

        /**
         * @brief Add multiple root states from an iterable structure.
   	 *
         */
        template <typename Iterable>
        void add_root_states(const Iterable& root_states)
        {
            root_states.insert(root_states);
        }

        /**
         * @brief Add multiple root states from an initializer list.
         *
         */
        void add_root_states(const std::initializer_list<State> list)
        {
            root_states.insert(list);
        }

        /**
         * @brief Add a transition to the automaton. Ignores duplicates.
         */
        void add_transition(const Transition& transition) { delta.add(transition); }

        /**
         * @brief Add a transition given its source states, symbol and target states.
         */
        void add_transition(State source, const Symbol symbol, const std::vector<State>& targets)
        {
            const Transition transition(symbol, source, targets);
            delta.add(transition);
        }

        /**
         * @brief Check whether a state exists in the automaton.
         */
        bool contains_state(const State& state) const { return state < num_of_states; }

        /**
         * @brief Check whether a state is root (initial or final.
         */
        bool is_state_root(const State& state) const { return root_states.contains(state); }

        /**
         * @brief Print the automaton in a parsable mata format.
         */
        void print_mata(std::ostream& os) const;

        /**
         * @brief Print the automaton in an easy to read format.
         */
        void print_readable(std::ostream& os) const;

        unsigned get_num_of_states() const { return num_of_states; }
        const utils::SparseSet<State>& get_root_states() const { return root_states; }

        bool operator== (const Nfta& other) const;
    };
}
#endif // MATA_NFTA
