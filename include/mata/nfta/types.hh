/**
 * @file types.hh
 * @brief
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
    };

    using State = unsigned;
    using StateVectorSet = utils::OrdVector<std::vector<State>>;
    constexpr Symbol EPSILON{ std::numeric_limits<Symbol>::max() };

    /**
     * @brief
     */
    struct Transition
    {
        Symbol symbol;
        State source;
        std::vector<State> targets;

        explicit Transition(
            const Symbol symbol = {},
            const State source = {},
            const std::vector<State>& targets = {}
        )
            : symbol(symbol),
              source(source),
              targets(targets)
        {
        }

        bool operator<(const Transition& other) const
        {
            if (source != other.source) return source < other.source;
            if (symbol != other.symbol) return symbol < other.symbol;
            if (targets.size() != other.targets.size()) return targets.size() < other.targets.size();
            return targets < other.targets;
        }
    };
} // namespace mata::nfta
#endif //NFTA_TYPES_HH
