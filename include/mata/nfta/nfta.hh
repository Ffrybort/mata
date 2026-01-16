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
        utils::SparseSet<State> states;
        utils::SparseSet<State> initial_states;

        Alphabet* alphabet;
        ArityMap arities;

        Delta<Transition_top_down> delta;

    public:
        explicit Nfta(
            const utils::SparseSet<State>& init_states = {},
            const utils::SparseSet<State>& init_final_states = {},
            const Delta<Transition_top_down>& init_delta = {},
            Alphabet* init_alphabet = nullptr,
            ArityMap init_arities = {}
        )
            : states(init_states),
              initial_states(init_final_states),
              alphabet(init_alphabet),
              arities(std::move(init_arities)),
              delta(init_delta)
        {
        }

        /**
         * @brief Add a state to the automaton. Ignore duplicates.
         */
        void add_state(const State& state) { states.insert(state); }

        /**
         * @brief Add multiple states from an iterable structure.
         */
        template <typename Iterable>
        void add_states(const Iterable& new_states) { states.insert(new_states); }

        /**
         * @brief Add multiple states from an initializer list.
         */
        void add_states(std::initializer_list<State> list) { states.insert(list); }

        /**
         * @brief Add a state to both final states and states. Ignores duplicates.
         */
        void add_final_state(const State& state)
        {
            initial_states.insert(state);
            states.insert(state);
        }

        /**
         * @brief Add multiple final states from an iterable structure.
         */
        template <typename Iterable>
        void add_initial_states(const Iterable& initial_states)
        {
            initial_states.insert(states);
            states.insert(initial_states);
        }

        /**
         * @brief Add multiple final states from an initializer list.
         */
        void add_initial_states(const std::initializer_list<State> list)
        {
            initial_states.insert(list);
            states.insert(list);
        }

        /**
         * @brief Adds a transition to the automaton. Ignores duplicates.
         */
        void add_transition(const Transition_top_down& transition) { delta.add(transition); }

        /**
         * @brief Adds a transition given its source states, symbol and target states.
         */
        void add_transition(const Symbol symbol, State source, const std::vector<State>& targets)
        {
            const Transition_top_down transition(symbol, source, targets);
            delta.add(transition);
        }

        /**
         * @brief Checks whether a state exists in the automaton.
         */
        [[nodiscard]] bool contains_state(const State& state) const { return states.contains(state); }

        /**
         * @brief Checks whether a state is final.
         */
        [[nodiscard]] bool is_state_final(const State& state) const { return initial_states.contains(state); }

        /**
         * @brief Prints the automaton in a textual form.
         */
        void print(std::ostream& os) const;

        [[nodiscard]] const utils::SparseSet<State>& get_states() const { return states; }
        [[nodiscard]] const utils::SparseSet<State>& get_initial_states() const { return initial_states; }
        [[nodiscard]] std::set<Transition_top_down> get_transitions() const { return delta.get_transitions(); }
    };
}
#endif
