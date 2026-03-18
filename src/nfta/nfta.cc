#include <iostream>
#include <ostream>

#include "mata/nfta/nfta.hh"

namespace mata::nfta {

void print_transitions(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
    for (const auto& transition : transitions) {
        os <<  transition.source << " -> ";
        if (alphabet) { os << alphabet->reverse_translate_symbol(transition.symbol); }
        else { os << transition.symbol; }
        if (!transition.targets.empty()) {

            os <<"(";
            for (std::size_t i = 0; i < transition.targets.size(); ++i) {
                os << transition.targets[i];
                if (i + 1 < transition.targets.size()) os << ",";
            }
            os << ")";
        }
        os << std::endl;
    }
} // print_transitions

void print_transitions_bottom_up(std::ostream& os, const std::vector<Transition>& transitions, const Alphabet* alphabet) {
    for (const auto& transition : transitions) {
        os <<"(";
        for (std::size_t i = 0; i < transition.targets.size(); ++i) {
            os << transition.targets[i];
            if (i + 1 < transition.targets.size()) os << ",";
        }
        os << ")";

        os << " -> ";
        if (alphabet) { os << alphabet->reverse_translate_symbol(transition.symbol); }
        else { os << transition.symbol; }
        os << " " <<  transition.source;
        os << std::endl;
    }
} // print_transitions

void Nfta::print_mata(std::ostream& os) const {
    os << "@NFTA-explicit" << std::endl;
    os << "%States-marked " << std::endl;
    os << "%Alphabet-auto " << std::endl;
    os << "%Initial ";
    for (const State state : initial_states) { os << "q" << state << " "; }
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
} // print_mata


void Nfta::print_readable(std::ostream& os) const {
    os << "NFTA";
    os << "================================================" << std::endl;

    // States
    os << "States (" << delta.num_of_states() << "): ";

    // Initial states
    os << "Initial states: ";

    for (const State s : initial_states) {
        os << s << " ";
    }
    os << std::endl;

    // Alphabet
    os << "Alphabet:";
    if (alphabet) {
        try {
            for (const Symbol sym : alphabet->get_alphabet_symbols()) {
                os << "  "<< alphabet->try_reverse_translate_symbol(sym); // todo print arities
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
    print_transitions(os, delta.get_transitions(), alphabet);
    os << "\n================================================\n";
} // print_readable

void Nfta::print_readable_bottom_up(std::ostream& os) const {
    os << "NFTA";
    os << "================================================" << std::endl;

    // States
    os << "States (" << delta.num_of_states() << "): ";

    // Initial states
    os << "Final states: ";

    for (const State s : initial_states) {
        os << s << " ";
    }
    os << std::endl;

    // Alphabet
    os << "Alphabet:";
    if (alphabet) {
        try {
            for (const Symbol sym : alphabet->get_alphabet_symbols()) {
                os << "  "<< alphabet->try_reverse_translate_symbol(sym); // todo print arities
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
    print_transitions_bottom_up(os, delta.get_transitions(), alphabet);
    os << "\n================================================\n";
} // print_readable

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

    utils::SparseSet<State> new_inital_states;
    for (const State s : initial_states) {
        if (is_staying[s]) {
            assert(s < renaming.size());
            new_inital_states.insert(renaming[s]);
        }
    }
    initial_states = std::move(new_inital_states);
} // defragment

void Nfta::print_timbuk(std::ostream& os, const std::string& name) const {
    // Alphabet with correct arities
    os << "Ops ";
    auto symbols = delta.get_used_symbols_arities();

    for (const auto& [symbol, arity] : symbols) {
        std::string sym_str;

        if (alphabet) { sym_str = alphabet->try_reverse_translate_symbol(symbol); }
        else { sym_str = std::to_string(symbol); }

        os << sym_str << ":" << arity << " ";
    }
    os << std::endl;

    os << "Automaton " << name << std::endl;
    os << "States ";
    for (State s = 0; s < delta.num_of_states(); ++s) {
        os << "q" << s << " ";
    }
    os << std::endl;
    os << "Final States ";
    for (const State s : initial_states) {
        os << "q" << s << " ";
    }
    os << std::endl;

    os << "Transitions" << std::endl;

    for (const auto& t : delta.get_transitions()) {
        std::string sym_str;

        if (alphabet) { sym_str = alphabet->reverse_translate_symbol(t.symbol); }
        else { sym_str = std::to_string(t.symbol); }

        if (t.targets.empty()) {
            // constant
            os << "  " << sym_str << " -> q" << t.source << std::endl;
        } else {
            // function symbol
            os << "  " << sym_str << "(";
            for (size_t i = 0; i < t.targets.size(); ++i) {
                os << "q" << t.targets[i];
                if (i + 1 < t.targets.size()) os << ",";
            }
            os << ") -> q" << t.source << std::endl;
        }
    }
}

bool Nfta::operator== (const Nfta& other) const {
    return delta.num_of_states() == other.delta.num_of_states()
        && initial_states == other.initial_states
        && delta == other.delta
        && alphabet == other.alphabet;
} // operator==

bool Nfta::has_equal_structure (const Nfta& other) const {
    return delta.num_of_states() == other.delta.num_of_states()
        && initial_states == other.initial_states
        && delta == other.delta;
} // has_equal_structure

}
