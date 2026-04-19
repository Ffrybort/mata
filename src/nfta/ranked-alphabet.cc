#include "mata/nfta/ranked-alphabet.hh"

namespace mata::nfta {
mata::utils::OrdVector<SymbolArity> mata::nfta::RankedOnTheFlyAlphabet::get_alphabet_symbols_arities() const {
    utils::OrdVector<SymbolArity> result;
    result.reserve(symbol_map_.size());
    for (const auto& [key, value] : symbol_map_) {
        result.insert(SymbolArity{value, key.second});
    }
    return result;
}

utils::OrdVector<Symbol> RankedOnTheFlyAlphabet::get_constant_symbols() const {
    utils::OrdVector<Symbol> result; result.reserve(symbol_map_.size());
    for (const auto& [string_arity, symbol] : symbol_map_) {
        if (string_arity.second == 0) { result.insert(symbol); }
    }
    return result;
}

utils::OrdVector<Symbol> RankedOnTheFlyAlphabet::get_non_constant_symbols() const {
    utils::OrdVector<Symbol> result; result.reserve(symbol_map_.size());
    for (const auto& [string_arity, symbol] : symbol_map_) {
        if (string_arity.second > 0) { result.insert(symbol); }
    }
    return result;
}

mata::utils::OrdVector<Symbol> mata::nfta::RankedOnTheFlyAlphabet::get_alphabet_symbols() const {
    utils::OrdVector<Symbol> result; result.reserve(symbol_map_.size());
    for (const auto& symbol : symbol_map_ | std::views::values) {
        result.insert(symbol);
    }
    return result;
}

mata::utils::OrdVector<Symbol> mata::nfta::RankedOnTheFlyAlphabet::get_complement(const mata::utils::OrdVector<mata::Symbol>& symbols) const {
    return get_alphabet_symbols().difference(symbols);
}

std::string mata::nfta::RankedOnTheFlyAlphabet::reverse_translate_symbol(const mata::Symbol symbol) const {
    for (const auto& [symbol_name, symbol_val]: symbol_map_) {
        if (symbol_val == symbol) { // todo throw if more than one
            return symbol_name.first;
        }
    }
    throw std::runtime_error("symbol '" + std::to_string(symbol) + "' is out of range of enumeration");
}

void mata::nfta::RankedOnTheFlyAlphabet::add_symbols_from(const std::vector<StringArity>& symbols) {
    for (const StringArity& symbol: symbols) {
        add_new_symbol(symbol);
    }
}

void mata::nfta::RankedOnTheFlyAlphabet::add_symbols_from(const SymbolArityMap& new_symbol_map) {
    for (const auto& [key, value] : new_symbol_map) {
        // Only add if the symbol doesn't already exist
        if (!symbol_map_.contains(key)) {
            symbol_map_[key] = value;
            update_next_symbol_value(value);
        }
    }
}

void mata::nfta::RankedOnTheFlyAlphabet::add_new_symbol(const StringArity& key, Symbol value) {
    if(!symbol_map_.insert({ key, value}).second) { throw std::runtime_error("Adding symbol failed - already exists");}
    update_next_symbol_value(value);
}

void mata::nfta::RankedOnTheFlyAlphabet::try_add_new_symbol(const std::string& symbol, unsigned arity) {
    StringArity key(symbol, arity);
    symbol_map_.insert({ key, next_symbol_value_});
    update_next_symbol_value(next_symbol_value_);
}

size_t mata::nfta::RankedOnTheFlyAlphabet::erase(Symbol symbol) {
    for (auto it = symbol_map_.begin(); it != symbol_map_.end(); ++it) {
        if (it->second == symbol) {
            if (symbol == next_symbol_value_ - 1) {
                --next_symbol_value_;
            }
            symbol_map_.erase(it);
            return 1;
        }
    }
    return 0;
}

size_t mata::nfta::RankedOnTheFlyAlphabet::erase(const StringArity& symbol_name) {
    if (const auto found_it{ symbol_map_.find(symbol_name) }; found_it != symbol_map_.end()) {
        if (found_it->second == next_symbol_value_ - 1) { --next_symbol_value_; }
        symbol_map_.erase(found_it);
        return 1;
    }
    return 0;
}

size_t mata::nfta::RankedOnTheFlyAlphabet::contains_symbol_name(const std::string& str) {
    size_t res = 0;
    for (const auto& key: symbol_map_ | std::views::keys) {
        if (key.first == str) { res++;; }
    }
    return res;
}

// todo this makes it inconsistent with the other ranked alphabets, is it a problem?
std::vector<unsigned> mata::nfta::RankedOnTheFlyAlphabet::get_arity(Symbol symbol) const {
    std::vector<unsigned> result = {};
    for (const auto& [key, value] : symbol_map_) {
        if (value == symbol) { result.push_back(key.second); }
    }
    return result;
}

void mata::nfta::RankedOnTheFlyAlphabet::change_arity(Symbol symbol, unsigned new_arity) {
    for (auto it = symbol_map_.begin(); it != symbol_map_.end(); ++it) {
        if (it->second == symbol) {
            StringArity new_key{it->first.first, new_arity};
            symbol_map_[new_key] = it->second;
            symbol_map_.erase(it);
            return;
        }
    }
    throw std::runtime_error("Cannot set arity of a nonexistent symbol.");
}

Symbol mata::nfta::RankedOnTheFlyAlphabet::translate_or_add_symbol(const std::string &str, unsigned arity) {
    const auto [it, inserted] = symbol_map_.insert({{str, arity}, next_symbol_value_});
    if (inserted) {
        return next_symbol_value_++;
    }
    return it->second;
}

Symbol mata::nfta::RankedOnTheFlyAlphabet::translate_symbol(const std::string &str, unsigned arity) {
    auto it = symbol_map_.find({str, arity});
    if (it == symbol_map_.end()) {
        throw std::runtime_error("Symbol " + str + "(" + std::to_string(arity) + ") does not exist or has a different arity");
    }
    return it->second;
}
}