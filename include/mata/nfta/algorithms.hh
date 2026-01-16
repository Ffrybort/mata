/**
* @file algorithms.hh
 * @brief Automaton algorithms (not implemented yet)
 *
 * todo later:
 * membership of a term
 * emptiness of a recognized language
 * intersection non-emptiness
 * finiteness
 * complement, emptiness of the complement
 * equivalence of two automata
 * singleton set
 * some of these may live in nfta class
 */

#ifndef NFTA_ALGORITHMS_HH
#define NFTA_ALGORITHMS_HH

#include "nfta.hh"
#include <stdexcept>
namespace mata::nfta
{

    /**
     * @brief Checks if the automaton is deterministic.
     */
    bool isDeterministic(Nfta automaton);

    /**
     * @brief Checks if the automaton is minimal.
     */
    bool isMinimal(Nfta automaton);

    /**
     * @brief Checks if the automaton is complete.
     */
    bool isComplete(Nfta automaton);

    /**
     * @brief Minimizes the automaton.
     */
    void minimize(Nfta automaton);

    /**
     * @brief Determinizes the automaton.
     */
    void determinize(Nfta automaton);

    /**
     * @brief Completes the automaton.
     */
    void complete(Nfta automaton);

    /**
     * @brief Returns the union of two automata.
     */
    Nfta automatonUnion(Nfta aut1, Nfta aut2);

    /**
     * @brief Returns the complementation of an automaton.
     */
    Nfta automatonComplementation(Nfta aut1, Nfta aut2);

    /**
     * @brief Returns the intersection of two automata.
     */
    Nfta automatonIntersection(Nfta aut1, Nfta aut2);
}

#endif //NFTA_ALGORITHMS_HH
