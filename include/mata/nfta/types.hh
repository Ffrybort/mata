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

    /**
    * @brief A mapping of symbols to arities. Constants (arity 0) are not stored. Every symbol not stored is considered a constant.
    *
    */
    struct ArityMap // todo incorporate into alphabet?
    {
    public:
        std::unordered_map<Symbol, unsigned> arities_; ///< Maps function symbols to arities; constants not stored

        ArityMap() : arities_() {}
        void set_arity(Symbol symbol, unsigned arity) { if (arity > 0) { arities_[symbol] = arity; } }
        unsigned get_arity(Symbol symbol) const { return arities_.contains(symbol) ? arities_.at(symbol) : 0; }

        bool operator==(const ArityMap& other) const {
            if (arities_.size() != other.arities_.size()) { return false; }
            for (auto [symbol, arity] : arities_) {
                if (other.get_arity(symbol) != arity) { return false; }
            }
            return true;
        }
    };

    using State = unsigned;
    using StateVectorSet = utils::OrdVector<std::vector<State>>;
    constexpr Symbol EPSILON{ std::numeric_limits<Symbol>::max() };


} // namespace mata::nfta
#endif //NFTA_TYPES_HH
