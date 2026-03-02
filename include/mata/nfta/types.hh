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

    struct Limits {
      static constexpr State min_state = std::numeric_limits<State>::min();
      static constexpr State max_state = std::numeric_limits<State>::max();
      static constexpr Symbol min_symbol = std::numeric_limits<Symbol>::min();
      static constexpr Symbol max_symbol = std::numeric_limits<Symbol>::max();
    };

    constexpr Symbol EPSILON{ Limits::max_symbol };
    enum class ProductFinalStateCondition {
        And, ///< Both original states have to be final.
        Or,  ///< At least one of the original states has to be final.
    };

using StateRenaming = std::unordered_map<State, State>;
} // namespace mata::nfta
#endif //NFTA_TYPES_HH
