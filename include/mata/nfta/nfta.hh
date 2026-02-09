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
        utils::SparseSet<State> final_states; // a set of final (or initial) states
        Alphabet* alphabet;
        ArityMap arities;
        Delta delta; // states live in delta, so do functions like add_state()

    public:
        explicit Nfta(
            const utils::SparseSet<State>& final_states = {},
            Alphabet* alphabet = nullptr,
            ArityMap arities = {},
            Delta  delta = {}
        )
            :
              final_states(final_states),
              alphabet(alphabet),
              arities(std::move(arities)),
              delta(std::move(delta))
        {
        }
        Nfta(const Nfta&) = delete; // todo implement moving
        Nfta& operator=(const Nfta&) = delete;

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
         * @brief Check whether a state is final (initial).
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

        bool operator== (const Nfta& other) const;
    }; // class Nfta
} // namespace mata::nfta
#endif // MATA_NFTA_H
