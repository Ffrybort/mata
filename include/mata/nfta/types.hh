/**
 * @file types.hh
 * @brief
 */

#ifndef NFTA_TYPES_HH
#define NFTA_TYPES_HH

#include "mata/alphabet.hh"

namespace mata::nfta
{

/**
* @brief A mapping of symbols to arities. Constants (arity 0) are not stored. Every symbol not stored is considered a constant.
*
*/
struct ArityMap
{
public:
    std::unordered_map<Symbol, unsigned> arities_; ///< Maps function symbols to arities; constants not stored

    ArityMap() : arities_() {}
    void set_arity(Symbol symbol, unsigned arity) { if (arity > 0) { arities_[symbol] = arity; } }
    unsigned get_arity(Symbol symbol) const { return arities_.contains(symbol) ? arities_.at(symbol) : 0; }
};

using State = unsigned;
using StateVectorSet = utils::OrdVector<std::vector<State>>;


}

#endif //NFTA_TYPES_HH
