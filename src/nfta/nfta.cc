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
        else { os << "Top-down NFTA\n"; }


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
                    os << "  "<< alphabet->reverse_translate_symbol(sym); // todo print arities
                }
            } catch (const std::exception& e) {
                ;
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

    void Nfta::defragment(const BoolVector& is_staying) {
        std::vector<State> renaming(is_staying.size());
        State next_new_state = 0;
        for (State i = 0; i < is_staying.size(); ++i) {
            if (is_staying[i]) {
                renaming[i] = next_new_state;
                next_new_state++;
            }
        }
        delta.defragment(is_staying, renaming);

        utils::SparseSet<State> new_final_states;
        for (const State s : final_states) {
            if (is_staying[s]) {
                assert(s < renaming.size());
                new_final_states.insert(renaming[s]);
            }
        }
        final_states = std::move(new_final_states);
    }

    /**
     * @brief Check if the two automata are identical. Alphabets are not compared -
     * only that transitions use the same symbol representations.
     *
     */
    bool Nfta::operator== (const Nfta& other) const {
        return delta.num_of_states() == other.delta.num_of_states()
            && final_states == other.final_states
            && delta == other.delta;
    }
}
