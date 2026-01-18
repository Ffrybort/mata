#include <iostream>
#include <ostream>

#include "mata/nfta/nfta.hh"
namespace mata::nfta
{
    void Nfta::print_mata(std::ostream& os) const
    {
        os << "@NFTA_TD-explicit" << std::endl;
        os << "%states-marked " << std::endl;

        os << "%alphabet-marked ";

        os << std::endl << "%initial ";
        for (State state : initial_states) { os <<"q" << state << " "; }
        std::cout << std::endl;

        for (const auto& transition : delta)
        {
            // source symbol ->
            os << "q" << transition.source << " a" << alphabet->reverse_translate_symbol(transition.symbol) << " ";

            // targets
            for (const auto& target : transition.targets)
            {
                os << "q" << target << " ";
            }
            os << std::endl;
        }
    }

    void Nfta::print_readable(std::ostream& os) const
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
        for (State s : initial_states) {
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
        for (const auto& tr : delta) {
            os << "  q" << tr.source << " -> ";
            if (alphabet) { os << alphabet->reverse_translate_symbol(tr.symbol); }
            else { os << tr.symbol; }

            if (!tr.targets.empty()) {
                os << "(";
                for (std::size_t i = 0; i < tr.targets.size(); ++i) {
                    if (i > 0) os << ", ";
                    os << "q" << tr.targets[i];
                }
                os << ")";
            }
            os << std::endl;;
        }

        os << "================================================\n";
    }

}
