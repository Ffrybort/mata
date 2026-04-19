#ifndef MATA_RANKED_ALPHABET_HH
#define MATA_RANKED_ALPHABET_HH

#include "mata/nfta/types.hh"

namespace mata::nfta {
/**
 * The abstract interface for ranked alphabets
 */
class RankedAlphabet : public Alphabet {
public:
    using Alphabet::add_new_symbol;

    void try_add_new_symbol(const std::string& symbol) override { (void)symbol; throw std::runtime_error("Unimplemented"); }

    /// translates a string into a symbol
    virtual Symbol translate_symbol(const std::string &symbol, unsigned arity) = 0;

    /// also translates strings to symbols
    Symbol operator[](const StringArity& sa) { return this->translate_symbol(sa.first, sa.second); }

    /**
     * @brief Get a set of all symbols and their arities in the alphabet.
     *
     * The result does not have to equal the list of symbols in the automaton using this alphabet.
     */
    virtual utils::OrdVector<SymbolArity> get_alphabet_symbols_arities() const { throw std::runtime_error("Unimplemented"); }

    virtual utils::OrdVector<Symbol> get_non_constant_symbols() const { throw std::runtime_error("Unimplemented"); }

    virtual utils::OrdVector<Symbol> get_constant_symbols() const { throw std::runtime_error("Unimplemented"); }

    ~RankedAlphabet() override = default;

    virtual void add_new_symbol(const std::string& symbol, unsigned arity) = 0;
    virtual void try_add_new_symbol(const std::string& symbol, unsigned arity) = 0;

protected:
    const void* address() const override { return this; }
}; // class RankedAlphabet.

/**
 * @brief A ranked version of the OnTheFlyAlphabet. Allows multiple symbols of the same name with different arity.
 *
 * Symbols of the same name with different arities are internally represented as different symbol values.
 */
class RankedOnTheFlyAlphabet : public RankedAlphabet {
public:
    using SymbolArityMap = std::unordered_map<StringArity, Symbol>;
    using Alphabet::try_add_new_symbol;

    explicit RankedOnTheFlyAlphabet(const Symbol init_symbol = 0) : next_symbol_value_(init_symbol) {};
    RankedOnTheFlyAlphabet(const RankedOnTheFlyAlphabet& alphabet) = default;
    RankedOnTheFlyAlphabet(RankedOnTheFlyAlphabet&& alphabet) = default;
    explicit RankedOnTheFlyAlphabet(const RankedOnTheFlyAlphabet* const alphabet): RankedOnTheFlyAlphabet(*alphabet) {}
    explicit RankedOnTheFlyAlphabet(SymbolArityMap str_sym_map) : symbol_map_(std::move(str_sym_map)) {}

    /**
     * @brief Translate a symbol, add if missing.
     */
    Symbol translate_or_add_symbol(const std::string &str, unsigned arity);

    /**
     * @brief Translate a symbol.
     * @throws std::runtime_error if symbol is missing.
     */
    Symbol translate_symbol(const std::string &str, unsigned arity);

    /**
     * @brief Translate a symbol with implicit arity 0. todo delete this?
     * @throws std::runtime_error if symbol is missing.
     */
    Symbol translate_symb(const std::string &symb) override { return translate_symbol(symb, 0); }

    /**
     * @brief Create alphabet from a list of StringArity instances.
     * @param symbols StringArity instances (pair of std::string and unsigned).
     * @param init_symbol Start of a sequence of values to use for new symbols.
     */
    explicit RankedOnTheFlyAlphabet(const std::vector<StringArity>& symbols, const Symbol init_symbol = 0)
        :  next_symbol_value_(init_symbol) { add_symbols_from(symbols); }

    /**
     * Create alphabet from a list of symbol names and arities.
     * @param symbols Names for symbols.
     * @param arities Arities for symbols.
     * @param init_symbol Start of a sequence of values to use for new symbols.
     */
    explicit RankedOnTheFlyAlphabet(
        const std::vector<std::string>& symbols, std::vector<unsigned> arities, const Symbol init_symbol = 0)
        :  next_symbol_value_(init_symbol) {
        if (symbols.size() != arities.size()) { throw std::invalid_argument("symbols and arities sizes differ"); }
        for (size_t i = 0; i < symbols.size(); ++i) { add_new_symbol(StringArity{symbols[i], arities[i]}); }
    }

    /**
     * @brief Add symbols from an iterable of StringArity instances.
     */
    template <class InputIt> RankedOnTheFlyAlphabet(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            add_new_symbol(*first, next_symbol_value_);
        }
    }
    /**
     * @brief Add symbols from an iterable of StringArity (pair of std::string and unsigned).
     */
    RankedOnTheFlyAlphabet(std::initializer_list<std::pair<StringArity, Symbol>> name_symbol_map)  {
        for (auto&& [name, symbol]: name_symbol_map) {
            add_new_symbol(name, symbol);
        }
    }

    /**
     * @brief Get an OrdVector of all symbols with arities as SymbolArity (pair of Symbol and unsigned)
     *
     * This operation is slow as a new OrdVector is built.
     */
    utils::OrdVector<SymbolArity> get_alphabet_symbols_arities() const override;

    utils::OrdVector<Symbol> get_non_constant_symbols() const override;

    utils::OrdVector<Symbol> get_constant_symbols() const override;

    utils::OrdVector<Symbol> get_alphabet_symbols() const override;

    utils::OrdVector<Symbol> get_complement(const utils::OrdVector<Symbol>& symbols) const override;

    std::string reverse_translate_symbol(Symbol symbol) const override;

public:
    RankedOnTheFlyAlphabet& operator=(const RankedOnTheFlyAlphabet& rhs) = default;
    RankedOnTheFlyAlphabet& operator=(RankedOnTheFlyAlphabet&& rhs) = default;

    /**
     * @brief Expand alphabet by symbols from the passed @p symbols.
     *
     * Adding a symbol name which already exists will throw an exception.
     * @param[in] symbols Vector of symbol names and arities pairs.
     */
    void add_symbols_from(const std::vector<StringArity>& symbols);

    /**
     * @brief Expand alphabet by symbols from the passed @p symbol_map.
     *
     * The value of the already existing symbols will NOT be overwritten.
     * @param[in] symbol_map Map of strings + arities to symbols.
     */
    void add_symbols_from(const SymbolArityMap& symbol_map);
    /**
     * @brief Add new symbol with implicit arity 0.
     *
     * @throws std::runtime_error If the symbol already exists.
     */
    void add_new_symbol(const std::string& symbol) override {
      add_new_symbol(StringArity{symbol, 0}, next_symbol_value_);
    }

    /**
     * @brief Add new symbol to the alphabet with the value of @c next_symbol_value.
     * @throws std::runtime_error if the symbol was already present.
     * @param[in] key std::string and artiy pair.
     */
     void add_new_symbol(const StringArity& key) { add_new_symbol(key, next_symbol_value_); }

     void try_add_new_symbol(const std::string& symbol, unsigned arity) override;

    /**
     * @brief Add new symbol to the alphabet with the value of @c next_symbol_value.
     * @throws std::runtime_error if the symbol was already present.
     *
     * @param[in] str User-space representation of the symbol.
     * @param[in] arity of the symbol.
     */
    void add_new_symbol(const std::string& str, unsigned arity)override {
       add_new_symbol(StringArity{str, arity}, next_symbol_value_);
    }

    /**
     * @brief Add new symbol to the alphabet.
     * @throws std::runtime_error if the symbol was already present.
     *
     * @param[in] key User-space representation of the symbol.
     * @param[in] value Number of the symbol to be used on transitions.
     */
    void add_new_symbol(const StringArity& key, Symbol value);

    /**
     * @brief Add new symbol to the alphabet.
     * @throws std::runtime_error if the symbol was already present.
     *
     * @param[in] str User-space representation of the symbol.
     * @param[in] arity of the symbol.
     * @param[in] value Number of the symbol to be used on transitions.
     */
     void add_new_symbol(const std::string& str, unsigned arity, Symbol value) {
         add_new_symbol(StringArity{str, arity}, value);
     }

    /**
     * Get the next value for a potential new symbol. Value is NOT updated.
     */
    Symbol get_next_value() const { return next_symbol_value_; }

    /**
     * Get the number of existing symbols, epsilon symbols excluded.
     * @return The number of symbols.
     */
    size_t get_number_of_symbols() const { return symbol_map_.size(); }

    /**
     * Get the symbol map used in the alphabet.
     * @return Map mapping strings + arities to symbols used internally in Mata.
     */
    const SymbolArityMap& get_symbol_map() const { return symbol_map_; }

    bool empty() const override { return symbol_map_.empty(); }

    size_t contains_symbol_name(const std::string& str);

    std::vector<unsigned> get_arity(Symbol symbol) const;

    /**
     * @brief Change arity of an existing symbol.
     */
    void change_arity(Symbol symbol, unsigned new_arity);

private:
    SymbolArityMap symbol_map_{}; ///< Map of string  and arity transition symbols to symbol values.
    Symbol next_symbol_value_{}; ///< Next value to be used for a newly added symbol.

public:
    /**
     * @brief Update next symbol value when appropriate.
     *
     * When the newly inserted value is larger or equal to the current next symbol value, update the next symbol
     *  value to a value one larger than the new value.
     * @param value The value of the newly added symbol.
     */
    void update_next_symbol_value(Symbol value) { next_symbol_value_ = std::max(next_symbol_value_, value + 1); }

    /**
     * @brief Remove a symbol name value pair specified by its @p symbol from the alphabet.
     *
     * @warning Complexity: O(n), where n is the number of symbols in the alphabet.
     * @return Number of symbols removed (0 or 1).
     */
    size_t erase(Symbol symbol);

    /**
     * @brief Remove a symbol name value pair specified by its @p symbol_name from the alphabet.
     * @return Number of symbols removed (0 or 1).
     */
    size_t erase(const StringArity& symbol_name);

    size_t erase(const std::string& symbol_name, unsigned arity) { return erase(StringArity{symbol_name, arity}); }

    /**
     * @brief Remove a symbol name value pair from the position @p pos from the alphabet.
     */
    void erase(const SymbolArityMap::const_iterator pos) { symbol_map_.erase(pos); }

    /**
     * @brief Remove a symbol name value pair from the positions between @p first and @p last from the alphabet.
     */
    void erase(const SymbolArityMap::const_iterator first, const SymbolArityMap::const_iterator last) {
        symbol_map_.erase(first, last);
    }

    void clear() override { symbol_map_.clear(); next_symbol_value_ = 0; }
}; // class RankedOnTheFlyAlphabet.

//
// /** todo
//  * This alphabet needs some rethinking if it is to be used.
//  */
// class RankedEnumAlphabet : public Alphabet {
// public:
//     using SymbolArity = std::pair<Symbol, unsigned>;
//     explicit RankedEnumAlphabet() = default;
//     RankedEnumAlphabet(const RankedEnumAlphabet& alphabet) = default;
//     explicit RankedEnumAlphabet(const RankedEnumAlphabet* const alphabet): RankedEnumAlphabet(*alphabet) {}
//     RankedEnumAlphabet(RankedEnumAlphabet&& rhs) = default;
//
//     utils::OrdVector<SymbolArity> get_alphabet_symbols_arities() const override;
//     utils::OrdVector<Symbol> get_alphabet_symbols() const override;
//     utils::OrdVector<Symbol> get_complement(const utils::OrdVector<Symbol>& symbols) const override {
//         return get_alphabet_symbols().difference(symbols);
//     }
//
//     std::string reverse_translate_symbol(Symbol symbol) const override;
//
//     RankedEnumAlphabet& operator=(const RankedEnumAlphabet& rhs) = default;
//     RankedEnumAlphabet& operator=(RankedEnumAlphabet&& rhs) = default;
//
//     /**
//      * Add symbols from an iterable structure, ignore duplicates.
//      */
//     template <class InputIt> void add_symbols_from(InputIt first, InputIt last) {
//        for (; first != last; ++first) {
//           if (!symbols_.contains(first->first)) {
//               add_new_symbol(first->first, first->second);
//           }
//        }
//     }
//
//     /**
//      * @brief Expand alphabet by symbols from the passed @p alphabet. Ignore duplicates or invalid inputs.
//      */
//     void add_symbols_from(const RankedEnumAlphabet& alphabet) {
//         auto symbols_to_add = alphabet.get_alphabet_symbols_arities(); // keep it alive
//         add_symbols_from(symbols_to_add.begin(), symbols_to_add.end());
//     }
//
//     Symbol translate_symb(const std::string& str) override;
//
//     /**
//      * @brief Add new symbol to the alphabet with the value identical to its string representation.
//      *
//      * @param[in] symbol User-space representation of the symbol.
//      * @return Result of the insertion as @c InsertionResult.
//      */
//     void add_new_symbol(const std::string& str, unsigned arity);
//
//     /**
//      * @brief Add new symbol to the alphabet.
//      *
//      * @param[in] symbol Numeric value of the symbol.
//      * @param[in] arity of the symbol.
//      * @return Result of the insertion as @c InsertionResult.
//      */
//     void add_new_symbol(Symbol symbol, unsigned arity);
//
//
//     /**
//      * Get the next value for a potential new symbol.
//      * @return Next Symbol value.
//      */
//     Symbol get_next_value() const { return next_symbol_value_; }
//
//     /**
//      * Get the number of existing symbols, epsilon symbols excluded.
//      * @return The number of symbols.
//      */
//     size_t get_number_of_symbols() const { return symbols_.size(); }
//
//     bool empty() const override { return symbols_.empty(); }
//
// private:
//     std::unordered_map<Symbol, unsigned> symbols_{}; ///< Vector of symbol + arity pairs..
//     Symbol next_symbol_value_{ 0 }; ///< Next value to be used for a newly added symbol.
//
// public:
//     /**
//      * @brief Update next symbol value when appropriate.
//      *
//      * When the newly inserted value is larger or equal to the current next symbol value, update the next symbol
//      *  value to a value one larger than the new value.
//      * @param value The value of the newly added symbol.
//      */
//     void update_next_symbol_value(Symbol value) { next_symbol_value_ = std::max(next_symbol_value_, value + 1); }
//
//     /**
//      * @brief Erase a symbol from the alphabet.
//      * @return Number of symbols erased (0 or 1).
//      */
//     size_t erase(Symbol symbol) { return symbols_.erase(symbol); }
//
//     /**
//      * @brief Remove a symbol name value pair from the position @p pos from the alphabet.
//      * @return Iterator following the last removed element.
//      */
//     void erase(const std::unordered_map<Symbol, unsigned>::const_iterator pos) { symbols_.erase(pos); }
//
//     /**
//      * @brief Remove a symbol name value pair from the positions between @p first and @p last from the alphabet.
//      * @return Iterator following the last removed element.
//      */
//     void erase(const std::unordered_map<Symbol, unsigned>::const_iterator first,
//                const std::unordered_map<Symbol, unsigned>::const_iterator last) {
//         symbols_.erase(first, last);
//     }
//
//     void clear() override { symbols_.clear(); next_symbol_value_ = 0; }
//     Symbol translate_or_add_symbol(const std::string &str, unsigned arity) override;
//     Symbol translate_symbol(const std::string &str, unsigned arity) override;
// }; //RankedEnumAlphabet

}
#endif //RANKED_ALPHABET_HH
