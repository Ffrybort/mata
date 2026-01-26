#include <iostream>
#include <ostream>

#include "mata/nfta/nfta.hh"




namespace mata::nfta
{
    void print_transitions_bu(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions)
        {
            // sources
            for (const auto& target : transition.tuple)
            {
                os << "q" << target << " ";
            }
            // symbol target
            os << " a" << alphabet->reverse_translate_symbol(transition.symbol) << " q" << transition.single << std::endl;
        }
    }

    void print_transitions_td(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions)
        {
            // source symbol
            os << "q" << transition.single << " a" << alphabet->reverse_translate_symbol(transition.symbol) << " ";

            // targets
            for (const auto& target : transition.tuple)
            {
                os << "q" << target << " ";
            }
            os  << std::endl;
        }
    }

    void print_transitions_unknown(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions)
        {
            // ((tuple), symbol, single)
            os << "((";
            for (const auto& target : transition.tuple)
            {
                os << "q" << target << " ";
            }
            os << ") a" << alphabet->reverse_translate_symbol(transition.symbol);

            os << "q" << transition.single << ")" << std::endl;
        }
    }

    void Nfta::print_mata(std::ostream& os) const // todo change to type
    {
        os << "@NFTA_TD-explicit" << std::endl;
        os << "%States-marked " << std::endl;
        os << "%Alphabet-marked " << std::endl;

        os << "%Initial ";
        for (State state : root_states) { os <<"q" << state << " "; }
        os << std::endl;

        switch (type) {
            case TopDown:
                print_transitions_td(os, delta.get_transitions(), alphabet);
                break;
            case BottomUp:
                print_transitions_bu(os, delta.get_transitions(), alphabet);
                break;
            default:
                print_transitions_unknown(os, delta.get_transitions(), alphabet);
                break;
        }

    }

    void Nfta::print_readable(std::ostream& os) const // todo
	{
        os << "Top-down NFTA\n";
        os << "================================================\n";

        // States
        os << "States (" << num_of_states << "): ";
        for (State s = 0; s < num_of_states; ++s) {
            os << "q" << s << " ";
        }
        os << std::endl;

        // Initial states
        os << "Initial states: ";
        for (State s : root_states) {
            os << "q" << s << " ";
        }
        os << std::endl;

        // Alphabet
        os << "Alphabet:";
        if (alphabet) {
            try {
                for (Symbol sym : alphabet->get_alphabet_symbols()) {
                    os << "  "<< alphabet->reverse_translate_symbol(sym) << ":" << arities.get_arity(sym);
                }
            } catch (const std::exception& e) {
                os << " (only positive arities)";
                for (const auto& [sym, arity] : arities.arities_) {
                    os << " "<< alphabet->reverse_translate_symbol(sym) << ":" << arity;
                }
            }
        } else {
            os << "  none\n";
        }
        os << std::endl;

        // Transitions
        os << "Transitions:\n";
        os << "todo";

        os << "================================================\n";
    }

    /**
     * @brief Check if the two automata are identical.
	 * todo equality of language check?
     */
    bool Nfta::operator== (const Nfta& other) const {
        return num_of_states == other.num_of_states
            && root_states == other.root_states
            && delta == other.delta
            && arities.arities_ == other.arities.arities_
            && (
                alphabet == other.alphabet || // same pointer
                (alphabet && other.alphabet && alphabet->is_equal(other.alphabet))
            );
    }
}
