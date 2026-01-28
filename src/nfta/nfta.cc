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

    void Nfta::print_mata(std::ostream& os) const
    {
        os << "@NFTA-explicit" << std::endl;
        os << "%States-marked " << std::endl;
        os << "%Alphabet-auto " << std::endl;
        os << "%Initial ";
        for (State state : root_states) { os <<"q" << state << " "; }
        os << std::endl;

        for (const auto& transition : delta.get_transitions())
        {
            // source symbol
            os << "q" << transition.single << " " << alphabet->reverse_translate_symbol(transition.symbol) << " ";

            // targets
            os << "(";
            for (const auto& target : transition.tuple)
            {
                os << "q" << target << " ";
            }
            os << ")";
            os  << std::endl;
        }
    }

    void Nfta::print_readable(std::ostream& os, std::string type) const
	{
        if (type == "bottom-up") {os << "Bottom-up NFTA"; }
        else { os << "Top-down NFTA"; }


        os << "================================================" << std::endl;

        // States
        os << "States (" << num_of_states << "): ";

        // Root states
        if (type == "bottom-up") {os << "Final states: "; }
        else { os << "Initial states: "; }

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
        if (type == "bottom-up") { print_transitions_bu(os, delta.get_transitions(), alphabet); }
        else { print_transitions_td(os, delta.get_transitions(), alphabet); }
        os << "\n================================================\n";
    }

    /**
     * @brief Check if the two automata are identical (not equal).
     *
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
