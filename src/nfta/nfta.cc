#include <iostream>
#include <ostream>

#include "mata/nfta/nfta.hh"

namespace mata::nfta
{
    void print_transitions_bu(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions) {
            os << alphabet->reverse_translate_symbol(transition.symbol);
            if (transition.targets.size() > 0) {
                os << "(";
                for (std::size_t i = 0; i < transition.targets.size(); ++i) {
                    os << "q" << transition.targets[i];
                    if (i + 1 < transition.targets.size()) {
                        os << ",";
                    }
                }
                os << ")";
            }
            os << " -> " << "q" << transition.source << "\n";
        }
    }

    void print_transitions_td(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
        for (const auto& transition : transitions) {
            os <<  "q" <<  transition.source << " -> " << alphabet->reverse_translate_symbol(transition.symbol);
            if (transition.targets.size() > 0) {

                os <<"(";
                for (std::size_t i = 0; i < transition.targets.size(); ++i) {
                    os << "q" << transition.targets[i];
                    if (i + 1 < transition.targets.size()) os << ",";
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
        for (State state : final_states) { os << "q" << state << " "; }
        os << std::endl;

        for (const auto& transition : delta.get_transitions())
        {
            os << "q" << transition.source << " ";

            // symbol
            if (alphabet != nullptr) {
                os << alphabet->reverse_translate_symbol(transition.symbol);
            } else {
                os << transition.symbol;
            }
            os << " ";

            // targets
            os << "(";
            for (const auto& target : transition.targets)
            {
                os << "q" << target << " ";
            }
            os << ")";
            os << std::endl;
        }
    }


    void Nfta::print_readable(std::ostream& os, const std::string& type) const
	{
        if (type == "bottom-up") {os << "Bottom-up NFTA"; }
        else { os << "Top-down NFTA"; }


        os << "================================================" << std::endl;

        // States
        os << "States (" << delta.num_of_states() << "): ";

        // Final (or initial) states
        if (type == "bottom-up") {os << "Final states: "; }
        else { os << "Initial states: "; }

        for (State s : final_states) {
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
     * @brief Check if the two automata are identical. Alphabets are not compared -
     * only that transitions use the same symbol representations.
     *
     */
    bool Nfta::operator== (const Nfta& other) const {
        return delta.num_of_states() == other.delta.num_of_states()
            && final_states == other.final_states
            && delta == other.delta
            && arities.arities_ == other.arities.arities_;
    }
}
