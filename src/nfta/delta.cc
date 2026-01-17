#include "mata/nfta/delta.hh"

//mata::nfta::SymbolPost& mata::nfta::SymbolPost::operator=(SymbolPost&& rhs) noexcept {
//    symbol = rhs.symbol;
//    targets = std::move(rhs.targets);
//    return *this;
//}
//
//bool mata::nfta::SymbolPost::contains(const State s) const {
//    for (const auto& state_vector : targets) {
//        if (std::ranges::find(state_vector, s) != state_vector.end()) { return true; }
//    }
//    return false;
//}
//
//size_t mata::nfta::SymbolPost::count(const State s) const {
//    size_t total = 0;
//    for (const auto& vec : targets) { total += std::ranges::count(vec, s); }
//    return total;
//}
//
//bool mata::nfta::SymbolPost::contains(const std::vector<State>& state_vector) const {
//    return targets.find(state_vector) != targets.end();
//}
