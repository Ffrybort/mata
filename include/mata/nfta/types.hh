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
    constexpr Symbol EPSILON{ std::numeric_limits<Symbol>::max() };
} // namespace mata::nfta
#endif //NFTA_TYPES_HH
