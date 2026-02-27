/**
 *
 */

#ifndef NFTA_OPERATIONS_HH
#define NFTA_OPERATIONS_HH

#include <mata/nfta/nfta.hh>
#include <mata/nfta/types.hh>

#include <stdexcept>

namespace mata::nfta {

std::vector<StateSet> get_epsilon_closures(const Delta& delta, Symbol epsilon = EPSILON);

/**
 * @brief Assume the given symbol is epsilon and remove it. Symbol must be unary.
 */
Nfta remove_epsilon(const Nfta& aut, Symbol epsilon = EPSILON);
}

#endif //NFTA_OPERATIONS_HH
