/**
* @file builder.hh
* @brief A builder for top-down NFTA.
*/

#ifndef NFTA_BUILDER_HH
#define NFTA_BUILDER_HH

#include <mata/alphabet.hh>
#include <mata/nfta/td/nfta.hh>
#include <mata/nfta/types.hh>
#include <mata/parser/inter-aut.hh>

namespace mata::nfta {
    using NameStateMap = std::unordered_map<std::string, State>;

    /**
     * @brief Parse top-down nfta from an intermediate automaton.
     *
     * @param inter_aut Intermediate automaton to parse.
     * @param IntAlphabet, OnTheFlyAlphabet, or EnumAlphabet that is already filled with symbols.
     * @throws std::runtime_errro Alphabet is invalid or parsing fails.
     */
    Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, Alphabet *alphabet);

    /**
     *  @brief Parse from the mata nfta format in an input stream.
     *
     * @throws std::runtime_error Parsing fails.
     */
    Nfta parse_from_mata(std::istream& input, Alphabet *alphabet);

    /**
     * @brief Parse from the mata nfta format in a string.
     *
     * @throws std::runtime_error Parsing fails.
     */
    Nfta parse_from_mata(const std::string& input, Alphabet *alphabet);
}
#endif //NFTA_BUILDER_HH
