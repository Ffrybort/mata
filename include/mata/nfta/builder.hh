/**
 * @file builder.hh
 * @brief A builder for NFTA.
 */

#ifndef NFTA_BUILDER_HH
#define NFTA_BUILDER_HH

#include <mata/alphabet.hh>
#include <mata/nft/nft.hh>
#include <mata/nfta/nfta.hh>
#include <mata/nfta/ranked-alphabet.hh>
#include <mata/nfta/types.hh>
#include <mata/parser/inter-aut.hh>
#include <mata/parser/parser.hh>

namespace mata::nfta {
using NameStateMap = std::unordered_map<std::string, State>;

/**
 * @brief Transition with the symbol as a string to be translated later.
 */
struct RawTransition {
    State source;
    std::string symbol_string;
    std::vector<State> targets;

    RawTransition() : source{}, symbol_string{}, targets{} {}
};

inline Nfta create_empty(Alphabet* alphabet = nullptr) { return Nfta({0}, alphabet, Delta(1)); }

/**
 * @brief Create an automaton accepting any tree build from the alphabet.
 */
Nfta create_universal(RankedAlphabet* alphabet);

/**
 * @brief Create an automaton accepting any tree build from the symbols.
 *
 * @param symbols [in] Symbols and arities used in the language.
 * @param alphabet [in, optional] Alphabet is only assigned to the automaton and otherwise left alone.
 */
Nfta create_universal(const utils::OrdVector<SymbolArity>* symbols, Alphabet* alphabet = nullptr);

/**
 * @brief Helper function to extract a transition from an inter_aut.
 * @param formula_node A node containing the left-hand side (source state).
 * @param formula_graph A graph containing the right-hand side (symbol and target(s)).
 * @param alphabet An alphabet to translate the symbol
 * @param state_map A mapping of state names to numbers used by the constructor.
 * @return A transition, with the symbol remaining a std::string, and states translated to internal numeric values.
 *
 * The reason for not translating a symbol right away is there is currently no unified way to do so for every possible
 * construction.
 */
RawTransition get_transition(
        const FormulaNode& formula_node, const FormulaGraph& formula_graph, Alphabet& alphabet,
        const NameStateMap& state_map);

/**
 * @brief Parse nfta from an intermediate automaton.
 *
 * @param inter_aut Intermediate automaton to parse.
 * @param alphabet a valid alphabet or nullptr (in that case IntAlphabet is created).
 * @throws std::runtime_error If parsing fails.
 */
Nfta construct_from_inter_aut(const IntermediateAut* inter_aut, Alphabet* alphabet = nullptr);

/**
 * @brief Parse nfta from an intermediate automaton, using a RankedOnTheFlyAlphabet allows for symbol overload.
 *
 * @param inter_aut Intermediate automaton to parse.
 * @param alphabet If it is nullptr, a new RankedOnTheFlyAlphabet is created and its deletion is up to the user.
 * @throws std::runtime_error If parsing fails.
 */
Nfta construct_from_inter_aut(const IntermediateAut* inter_aut, RankedOnTheFlyAlphabet* alphabet = nullptr);

/**
 * @brief Parse nfta from a parsed object.
 * @throws std::runtime_error Alphabet is invalid or parsing fails.
 */
Nfta construct_from_parsed_object(const parser::Parsed* parsed, Alphabet* alphabet);

/**
 *  @brief Parse from the mata nfta format in an input stream.
 * @throws std::runtime_error Alphabet is invalid or parsing fails.
 */
Nfta parse_from_mata(std::istream& input, Alphabet* alphabet);

/**
 * @brief Parse from the mata nfta format in a string.
 * @throws std::runtime_error Alphabet is invalid or parsing fails.
 */
Nfta parse_from_mata(const std::string& input, Alphabet* alphabet);
} // namespace mata::nfta
#endif // NFTA_BUILDER_HH
