/**
 * @file nfta.cc
 *
 * @brief Implementation of NFTA output and helper functions.
 *
 * Copyright (C) 2026, Felix Frybort.
 */

#include <cctype>
#include <ostream>
#include <string>

#include "mata/nfta/nfta.hh"

namespace mata::nfta {

void Nfta::print_mata(std::ostream& os) const {
    os << "@NFTA-explicit" << std::endl;
    os << "%States-marked " << std::endl;
    os << "%Alphabet-auto " << std::endl;
    os << "%Initial ";
    for (const State state : root_states) {
        os << "q" << state << " ";
    }
    os << std::endl;

    for (const auto& transition : delta.get_transitions()) {
        os << "q" << transition.single << " ";

        // symbol
        if (alphabet != nullptr) {
            os << alphabet->reverse_translate_symbol(transition.symbol);
        } else {
            os << transition.symbol;
        }
        os << " ";

        // targets
        os << "(";
        for (const auto& target : transition.tuple) {
            os << "q" << target << " ";
        }
        os << ")";
        os << std::endl;
    }
} // print_mata

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

    utils::SparseSet<State> new_root_states;
    for (const State s : root_states) {
        if (is_staying[s]) {
            assert(s < renaming.size());
            new_root_states.insert(renaming[s]);
        }
    }
    root_states = std::move(new_root_states);
} // defragment

void sanitize_symbol(std::string& s) {
    if (s.empty()) {
        s = "a";
        return;
    }
    // replace wierd characters with '_'
    for (char& c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            c = '_';
        }
    }
    // prefix with 'a' if needed
    if (!std::isalpha(static_cast<unsigned char>(s[0]))) {
        s = "a" + s;
    }
}

void Nfta::print_timbuk(std::ostream& os, const std::string& name) const {
    // Alphabet with correct arities
    os << "Ops ";
    auto symbols = delta.get_used_symbols_arities();

    for (const auto& [symbol, arity] : symbols) {
        std::string sym_str;

        if (alphabet) {
            sym_str = alphabet->try_reverse_translate_symbol(symbol);
        } else {
            sym_str = std::to_string(symbol);
        }
        sanitize_symbol(sym_str);

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
    for (const State s : root_states) {
        os << "q" << s << " ";
    }
    os << std::endl;

    os << "Transitions" << std::endl;

    for (const auto& t : delta.get_transitions()) {
        std::string sym_str;

        if (alphabet) {
            sym_str = alphabet->reverse_translate_symbol(t.symbol);
        } else {
            sym_str = std::to_string(t.symbol);
        }
        sanitize_symbol(sym_str);

        if (t.tuple.empty()) {
            // constant
            os << "  " << sym_str << " -> q" << t.single << std::endl;
        } else {
            // function symbol
            os << "  " << sym_str << "(";
            for (size_t i = 0; i < t.tuple.size(); ++i) {
                os << "q" << t.tuple[i];
                if (i + 1 < t.tuple.size())
                    os << ",";
            }
            os << ") -> q" << t.single << std::endl;
        }
    }
}

bool Nfta::is_identical_to(const Nfta& other) const {
    return delta.num_of_states() == other.delta.num_of_states() && root_states == other.root_states &&
           delta == other.delta && alphabet == other.alphabet;
} // operator==

} // namespace mata::nfta
