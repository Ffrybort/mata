/**
 * @file types.hh
 * @brief Basic nfta types that will likely be used for both top-down and bottom up.
 */

#ifndef NFTA_TYPES_HH
#define NFTA_TYPES_HH

#include "mata/alphabet.hh"
#include <limits>

namespace mata::nfta
{
    using State = unsigned;
    using StateVectorSet = utils::OrdVector<std::vector<State>>;
    using StateSet = utils::OrdVector<State>;

    using SymbolArity = std::pair<Symbol, unsigned>;
    using StringArity = std::pair<std::string, unsigned>;

    inline utils::OrdVector<Symbol> collect_symbols(const utils::OrdVector<SymbolArity>& sa, const bool ignore_constants = false) {
        utils::OrdVector<Symbol> symbols;
        for (auto [symbol, arity] : sa) {
            if (!ignore_constants || arity > 0) { symbols.insert(symbol); }
        }
        return symbols;
    }


    struct Limits {
      static constexpr State min_state = std::numeric_limits<State>::min();
      static constexpr State max_state = std::numeric_limits<State>::max();
      static constexpr Symbol min_symbol = std::numeric_limits<Symbol>::min();
      static constexpr Symbol max_symbol = std::numeric_limits<Symbol>::max();
    };

    constexpr Symbol EPSILON{ Limits::max_symbol };

    enum class Condition {
        And,
        Or,
    };

using StateRenaming = std::unordered_map<State, State>;
} // namespace mata::nfta
#endif //NFTA_TYPES_HH
