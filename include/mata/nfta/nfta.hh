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
	/**
     * @brief A mapping of symbols to arities. Constants (arity 0) are not stored. Every symbol not stored is considered a constant.
     */
    struct ArityMap
    {
    public:
        std::unordered_map<Symbol, unsigned> arities_; ///< Maps function symbols to arities; constants not stored

        ArityMap() : arities_() {}
        void set_arity(Symbol symbol, unsigned arity) { if (arity > 0) { arities_[symbol] = arity; } }
        unsigned get_arity(Symbol symbol) const { return arities_.contains(symbol) ? arities_.at(symbol) : 0; }

        bool operator==(const ArityMap& other) const {
            return arities_ == other.arities_;
        }
    };

    class Nfta
    {
    public:
        unsigned num_of_states; // todo delete this, use delta
        utils::SparseSet<State> final_states; // a set of final (or initial) states
        Alphabet* alphabet;
        ArityMap arities;
        Delta delta;

    public:
        explicit Nfta(
            const unsigned num_of_states = 0,
            const utils::SparseSet<State>& final_states = {},
            Alphabet* alphabet = nullptr,
            ArityMap arities = {},
            const Delta& delta = {}
        )
            : num_of_states(num_of_states),
              final_states(final_states),
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
         * Add a specific @p state value.
		 *
		 *
         */
        void add_state(const State state) {
            if (state >= delta.num_of_states()) {
                delta.allocate(state + 1);
                num_of_states = state + 1;
            }
		}

        void add_final_state(const State state) {
            add_state(state);
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
            add_state(*std::max_element(states.begin(), states.end()));
            final_states.insert(states);
        }


        /**
         * @brief Check whether a state exists in the automaton.
         */
        bool contains_state(const State& state) const { return state < num_of_states; }

        /**
         * @brief Check whether a state is final (initial).
         */
        bool is_state_final(const State& state) const { return final_states.contains(state); }

        /**
         * @brief Print the automaton in a parsable mata format.
         */
        void print_mata(std::ostream& os) const;

        /**
         * @brief Print the automaton in an easy to read format.
         */
        void print_readable(std::ostream& os, std::string type) const;

        unsigned get_num_of_states() const { return num_of_states; }
        const utils::SparseSet<State>& get_final_states() const { return final_states; }

        bool operator== (const Nfta& other) const;
    };
}
#endif // MATA_NFTA
