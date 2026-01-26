#include <iostream>
#include <ostream>

#include "mata/nfta/nfta.hh"

namespace mata::nfta
{
    void print_transitions_bu(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions) {
            os << alphabet->reverse_translate_symbol(transition.symbol);
            if (transition.tuple.size() > 0) {
                os << "(";
                for (std::size_t i = 0; i < transition.tuple.size(); ++i) {
                    os << "q" << transition.tuple[i];
                    if (i + 1 < transition.tuple.size()) {
                        os << ",";
                    }
                }
                os << ")";
            }
            os << " -> " << "q" << transition.single << "\n";
        }
    }

    void print_transitions_td(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions) {
            os <<  "q" <<  transition.single << " -> " << alphabet->reverse_translate_symbol(transition.symbol);
            if (transition.tuple.size() > 0) {

                os <<"(";
                for (std::size_t i = 0; i < transition.tuple.size(); ++i) {
                    os << "q" << transition.tuple[i];
                    if (i + 1 < transition.tuple.size()) os << ",";
                }
                os << ")";
            }
            os << std::endl;
        }
    }

    void print_transitions_mata_bu(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions)
        {
            // sources
            for (const auto& target : transition.tuple)
            {
                os << "q" << target << " ";
            }
            // symbol target
            os << alphabet->reverse_translate_symbol(transition.symbol) << " q" << transition.single << std::endl;
        }
    }

    void print_transitions_mata_td(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions)
        {
            // source symbol
            os << "q" << transition.single << " " << alphabet->reverse_translate_symbol(transition.symbol) << " ";

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
        switch (type) {
            case TopDown:
                os << "@NFTA_TD-explicit" << std::endl;
                break;
            case BottomUp:
                os << "@NFTA_BU-explicit" << std::endl;
                break;
            default:
                os << "Unknown automaton type" << std::endl;
                return;
        }
        os << "%States-marked " << std::endl;
        os << "%Alphabet-auto " << std::endl;

        if (type == TopDown) { os << "%Initial "; }
        else { os << "%Final "; }

        for (State state : root_states) { os <<"q" << state << " "; }
        os << std::endl;

        if (type == TopDown) { print_transitions_mata_td(os, delta.get_transitions(), alphabet); }
        else { print_transitions_mata_bu(os, delta.get_transitions(), alphabet); }
    }

    void Nfta::print_readable(std::ostream& os) const
	{
        switch (type) {
            case TopDown:
                os << "Top-down NFTA" << std::endl;
            break;
            case BottomUp:
                os << "Bottom-up NFTA" << std::endl;
            break;
            default:
                os << "Unknown type NFTA" << std::endl;
            return;
        }

        os << "================================================" << std::endl;

        // States
        os << "States (" << num_of_states << "): ";

        // Root states
        switch (type) {
            case TopDown:
                os << "Initial states: ";
            break;
            case BottomUp:
                os << "Final states: ";
            break;
            default:
                os << "Root (initial or final) states: ";
            return;
        }

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
        switch (type) {
            case TopDown:
                print_transitions_td(os, delta.get_transitions(), alphabet);
            break;
            case BottomUp:
                print_transitions_bu(os, delta.get_transitions(), alphabet);
            break;
            default:
                print_transitions_unknown(os, delta.get_transitions(), alphabet);
        }

        os << "\n================================================\n";
    }

    /**
     * @brief Check if the two automata are identical.
     *
     */
    bool Nfta::operator== (const Nfta& other) const {
        return type == other.type
            && num_of_states == other.num_of_states
            && root_states == other.root_states
            && delta == other.delta
            && arities.arities_ == other.arities.arities_
            && (
                alphabet == other.alphabet || // same pointer
                (alphabet && other.alphabet && alphabet->is_equal(other.alphabet))
               );
    }
}
