/**
* @file builder.hh
* @brief A builder for top-down NFTA.
*/

#ifndef NFTA_BUILDER_HH
#define NFTA_BUILDER_HH

#include <mata/alphabet.hh>
#include <mata/nft/nft.hh>
#include <mata/nfta/nfta.hh>
#include <mata/nfta/types.hh>
#include <mata/parser/inter-aut.hh>
#include <mata/parser/parser.hh>

namespace mata::nfta {
    using NameStateMap = std::unordered_map<std::string, State>;

    inline Nfta create_empty(Alphabet *alphabet = nullptr) {
       return Nfta({0}, alphabet, Delta(1));
    }

    /**
     * @brief Parse nfta from an intermediate automaton.
     *
     * @param inter_aut Intermediate automaton to parse.
     * @param alphabet a valid alphabet or nullptr (in that case IntAlphabet is created).
     * @throws std::runtime_error If parsing fails.
     */
    Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, Alphabet *alphabet = nullptr);

    /**
     * @brief Parse nfta from an intermediate automaton, using a RankedOnTheFlyAlphabet allows for symbol overload.
     *
     * @param inter_aut Intermediate automaton to parse.
     * @param alphabet If it is nullptr, a new RankedOnTheFlyAlphabet is created and its deletion is up to the user.
     * @throws std::runtime_error If parsing fails.
     */
    Nfta construct_from_inter_aut(const IntermediateAut *inter_aut, RankedOnTheFlyAlphabet *alphabet = nullptr);

    /**
     * @brief Parse nfta from a parsed object.
     * @throws std::runtime_error Alphabet is invalid or parsing fails.
     */
    Nfta construct_from_parsed_object(const parser::Parsed *parsed, Alphabet *alphabet);

    /**
     *  @brief Parse from the mata nfta format in an input stream.
     * @throws std::runtime_error Alphabet is invalid or parsing fails.
     */
    Nfta parse_from_mata(std::istream& input, Alphabet *alphabet);

    /**
     * @brief Parse from the mata nfta format in a string.
     * @throws std::runtime_error Alphabet is invalid or parsing fails.
     */
    Nfta parse_from_mata(const std::string& input, Alphabet *alphabet);
} // namespace mata::nfta
#endif //NFTA_BUILDER_HH
