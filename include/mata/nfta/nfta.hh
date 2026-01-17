/**
 * @file nfta.hh
 * @brief Implementation of a top-down finite tree automaton.
 */

#ifndef NFTA_H
#define NFTA_H

#include <string>
#include <iostream>
#include <set>
#include <utility>
#include <vector>
#include <utility>

#include "types.hh"
#include <mata/alphabet.hh>
#include <mata/utils/sparse-set.hh>
#include "types.hh"
#include "delta.hh"

namespace mata::nfta
{
    class Nfta
    {
        unsigned num_of_states;
        utils::SparseSet<State> initial_states;

        Alphabet* alphabet;
        ArityMap arities;

        Delta delta;

    public:
        explicit Nfta(
            const unsigned num_of_states = 0,
            const utils::SparseSet<State>& initial_states = {},
            const Delta& delta = {},
            Alphabet* alphabet = nullptr,
            ArityMap arities = {}
        )
            : num_of_states(num_of_states),
              initial_states(initial_states),
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
         * @brief Add a state to the automaton. Ignore duplicates.
         */
        State add_state() { num_of_states++; return num_of_states - 1;  }

        /**
         * @brief Add a state to initial states. Ignores duplicates.
         *
         * todo keep consistency
         */
        void add_initial_state(const State& state)
        {
            initial_states.insert(state);
        }

        /**
         * @brief Add multiple initial states from an iterable structure.
   	     *
         * todo consistency
         */
        template <typename Iterable>
        void add_initial_states(const Iterable& initial_states)
        {
            initial_states.insert(initial_states);
        }

        /**
         * @brief Add multiple initial states from an initializer list.
         *
         * todo consistency
         */
        void add_initial_states(const std::initializer_list<State> list)
        {
            initial_states.insert(list);
        }

        /**
         * @brief Adds a transition to the automaton. Ignores duplicates.
         */
        void add_transition(const Transition& transition) { delta.add(transition); }

        /**
         * @brief Adds a transition given its source states, symbol and target states.
         */
        void add_transition(const Symbol symbol, State source, const std::vector<State>& targets)
        {
            const Transition transition(symbol, source, targets);
            delta.add(transition);
        }

        /**
         * @brief Checks whether a state exists in the automaton.
         */
        [[nodiscard]] bool contains_state(const State& state) const { return state < num_of_states; }

        /**
         * @brief Checks whether a state is initial.
         */
        [[nodiscard]] bool is_state_initial(const State& state) const { return initial_states.contains(state); }

        /**
         * @brief Prints the automaton in a textual form.
         */
        void print(std::ostream& os) const;

        const unsigned get_num_of_states() const { return num_of_states; }
        const utils::SparseSet<State>& get_initial_states() const { return initial_states; }
        std::set<Transition> get_transitions() const { return delta.get_transitions(); }
    };
}
#endif
