#include <iostream>
#include <ostream>

#include "nfta.hh"
namespace mata::nfta
{
    void Nfta::print(std::ostream& os) const
    {
        os << "Bottom up NFTA" << std::endl;
        os << "States: ";
        for (const auto& state : states) { os << state << " "; }

        os << std::endl << "Symbols: ";
        for (Symbol s : alphabet->get_alphabet_symbols()) {
            os << alphabet->reverse_translate_symbol(s)
               << ":" << arities.get_arity(s) << " ";
        }

        os << std::endl << "Final States: ";
        for (const auto& state : initial_states) { os << state << " "; }

        os << std::endl << "Transitions:\n";
        for (const auto& transition : delta)
        {
            // source symbol ->
            os << transition.source << " " << alphabet->reverse_translate_symbol(transition.symbol) << " -> ";

            // targets in ()
            if (!transition.targets.empty())
            {
                os << "(";
                bool is_first = true;
                for (const auto& source : transition.targets)
                {
                    if (!is_first) { os << ","; }
                    is_first = false;
                    os << source;
                }
                os << ") ";
            }
        }
    }
}
