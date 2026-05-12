/**
 * @file ranked-alphabet.cc
 *
 * @brief Testing ranked alphabets.
 *
 * Copyright (C) 2026, Felix Frybort.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "mata/alphabet.hh"
#include "mata/nfta/ranked-alphabet.hh"
#include "mata/nfta/types.hh"

using namespace mata;
using namespace mata::nfta;
using namespace mata::utils;

TEST_CASE("mata::RankedOnTheFlyAlphabet") {

    SECTION("empty alphabet basics") {
        RankedOnTheFlyAlphabet a{};
        CHECK(a.empty());
        CHECK(a.get_number_of_symbols() == 0);
        CHECK(a.get_next_value() == 0);
        CHECK(a.get_alphabet_symbols().empty());
        CHECK(a.get_alphabet_symbols_arities().empty());
    }

    SECTION("translate_or_add_ranked_symbol inserts and returns symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symbol("a", 2);
        CHECK(s == 0);
        CHECK(a.get_number_of_symbols() == 1);
        CHECK(a.get_next_value() == 1);
    }

    SECTION("translate_or_add_ranked_symbol ignores duplicits") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_symbol("a", 2);
        Symbol s2 = a.translate_or_add_symbol("a", 2);
        CHECK(s1 == s2);
        CHECK(a.get_number_of_symbols() == 1);
    }

    SECTION("same name different arity yields different symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_symbol("a", 1);
        Symbol s2 = a.translate_or_add_symbol("a", 2);
        CHECK(s1 != s2);
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("translate_ranked_symbol throws if missing") {
        RankedOnTheFlyAlphabet a{};
        CHECK_THROWS(a.translate_symbol("missing", 0));
    }

    SECTION("translate_ranked_symbol returns existing symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symbol("a", 3);
        CHECK(a.translate_symbol("a", 3) == s);
    }

    SECTION("initializer_list constructor (StringArity -> Symbol)") {
        RankedOnTheFlyAlphabet a{{{{"a", 1}, 5}, {{"b", 2}, 7}}};
        CHECK(a.translate_symbol("a", 1) == 5);
        CHECK(a.translate_symbol("b", 2) == 7);
        CHECK(a.get_next_value() == 8);
    }

    SECTION("vector<StringArity> constructor") {
        std::vector<StringArity> v{{"a", 1}, {"b", 2}};
        RankedOnTheFlyAlphabet a{v};
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("iterator constructor") {
        std::vector<StringArity> v{{"x", 0}, {"y", 1}};
        RankedOnTheFlyAlphabet a{v.begin(), v.end()};
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("get_alphabet_symbols_arities returns ordered vector") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("b", 1);
        a.translate_or_add_symbol("a", 2);
        auto syms = a.get_alphabet_symbols_arities();
        CHECK(syms.size() == 2);
    }

    SECTION("get_alphabet_symbols returns symbols only") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);
        a.translate_or_add_symbol("b", 2);
        auto syms = a.get_alphabet_symbols();
        CHECK(syms.size() == 2);
    }

    SECTION("reverse_translate_symbol works") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symbol("a", 1);
        CHECK(a.reverse_translate_symbol(s) == "a");
    }

    SECTION("get_arity returns correct arity") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symbol("a", 4);
        CHECK(a.get_arity(s) == std::vector<unsigned>{4});
    }

    SECTION("change_arity updates arity") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symbol("a", 1);
        a.change_arity(s, 3);
        CHECK(a.get_arity(s) == std::vector<unsigned>{3});
    }

    SECTION("erase by symbol removes entry") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symbol("a", 1);
        CHECK(a.erase(s) == 1);
        CHECK(a.empty());
    }

    SECTION("clear resets alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);
        a.translate_or_add_symbol("b", 2);
        a.clear();
        CHECK(a.empty());
        CHECK(a.get_number_of_symbols() == 0);
        CHECK(a.get_next_value() == 0);
    }

    SECTION("translate_or_add_ranked_symb adds and reuses symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_symbol("a", 2);
        Symbol s2 = a.translate_or_add_symbol("a", 2);
        CHECK(s1 == s2);
        CHECK(a.get_number_of_symbols() == 1);
    }

    SECTION("translate_symb throws if missing") {
        RankedOnTheFlyAlphabet a{};
        CHECK_THROWS(a.translate_symb("missing"));
    }

    SECTION("get_complement returns missing symbols") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_symbol("a", 0);
        Symbol s2 = a.translate_or_add_symbol("b", 0);

        utils::OrdVector<Symbol> subset{s1};
        auto complement = a.get_complement(subset);

        CHECK(complement.size() == 1);
        CHECK(*complement.begin() == s2);
    }


    SECTION("add_symbols_from(vector<StringArity>) inserts all symbols") {
        RankedOnTheFlyAlphabet a{};
        std::vector<StringArity> symbols{{"a", 1}, {"b", 2}};
        a.add_symbols_from(symbols);
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("add_symbols_from(SymbolArityMap) does not overwrite existing symbols") {
        RankedOnTheFlyAlphabet a{};
        Symbol existing = a.translate_or_add_symbol("a", 1);

        RankedOnTheFlyAlphabet::SymbolArityMap map{{{"a", 1}, 42}, {{"b", 2}, 7}};

        a.add_symbols_from(map);

        CHECK(a.translate_symbol("a", 1) == existing);
        CHECK(a.translate_symbol("b", 2) == 7);
    }

    SECTION("add_new_symbol throws on duplicate") {
        RankedOnTheFlyAlphabet a{};
        a.add_new_symbol("a", 1);
        CHECK_THROWS(a.add_new_symbol("a", 1));
    }

    SECTION("string + arity constructor throws on size mismatch") {
        std::vector<std::string> names{"a", "b"};
        std::vector<unsigned> arities{1};
        CHECK_THROWS(RankedOnTheFlyAlphabet{names, arities});
    }

    SECTION("erase by StringArity removes symbol") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);
        CHECK(a.erase(StringArity{"a", 1}) == 1);
        CHECK(a.empty());
    }

    SECTION("erase by name and arity removes symbol") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);
        CHECK(a.erase("a", 1) == 1);
        CHECK(a.empty());
    }

    SECTION("copy constructor copies alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);

        RankedOnTheFlyAlphabet b{a};
        CHECK(b.get_number_of_symbols() == 1);
        CHECK(b.translate_symbol("a", 1) == a.translate_symbol("a", 1));
    }

    SECTION("move constructor transfers alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);

        RankedOnTheFlyAlphabet b{std::move(a)};
        CHECK(b.get_number_of_symbols() == 1);
    }

    SECTION("pointer constructor") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);
        RankedOnTheFlyAlphabet b{&a};
        CHECK(b.get_number_of_symbols() == 1);
    }

    SECTION("copy assignment") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);

        RankedOnTheFlyAlphabet b{};
        b = a;
        CHECK(b.get_number_of_symbols() == 1);
    }

    SECTION("SymbolArityMap constructor") {
        RankedOnTheFlyAlphabet::SymbolArityMap map{{{"a", 1}, 5}, {{"b", 2}, 7}};

        RankedOnTheFlyAlphabet a{map};

        CHECK(a.translate_symbol("a", 1) == 5);
        CHECK(a.translate_symbol("b", 2) == 7);
    }

    SECTION("move assignment") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);

        RankedOnTheFlyAlphabet b{};
        b = std::move(a);

        CHECK(b.get_number_of_symbols() == 1);
    }

    SECTION("get_symbol_map returns internal map") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);

        const auto& map = a.get_symbol_map();
        CHECK(map.size() == 1);
    }

    SECTION("update_next_symbol_value updates correctly") {
        RankedOnTheFlyAlphabet a{};

        a.update_next_symbol_value(10);
        CHECK(a.get_next_value() == 11);

        a.update_next_symbol_value(5); // should not decrease
        CHECK(a.get_next_value() == 11);
    }

    SECTION("erase by iterator removes symbol") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);

        auto it = a.get_symbol_map().begin();
        a.erase(it);

        CHECK(a.empty());
    }

    SECTION("erase iterator range removes symbols") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 1);
        a.translate_or_add_symbol("b", 2);

        auto first = a.get_symbol_map().begin();
        auto last = a.get_symbol_map().end();

        a.erase(first, last);

        CHECK(a.empty());
    }

    SECTION("add_new_symbol with explicit value") {
        RankedOnTheFlyAlphabet a{};

        a.add_new_symbol(StringArity{"a", 1}, 42);

        CHECK(a.translate_symbol("a", 1) == 42);
    }

    SECTION("add_new_symbol string arity value overload") {
        RankedOnTheFlyAlphabet a{};

        a.add_new_symbol("a", 2, 99);

        CHECK(a.translate_symbol("a", 2) == 99);
    }

    SECTION("get_non_constant_symbols returns only non-constant symbols") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 0);
        a.translate_or_add_symbol("f", 1);
        a.translate_or_add_symbol("p", 2);

        auto non_const = a.get_non_constant_symbols();
        auto sym_f = a.translate_symbol("f", 1);
        auto sym_p = a.translate_symbol("p", 2);
        auto sym_a = a.translate_symbol("a", 0);

        CHECK(non_const.contains(sym_f));
        CHECK(non_const.contains(sym_p));
        CHECK_FALSE(non_const.contains(sym_a));
        CHECK(non_const.size() == 2);
    }

    SECTION("get_constant_symbols returns only constants") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 0);
        a.translate_or_add_symbol("b", 0);
        a.translate_or_add_symbol("f", 1);

        auto consts = a.get_constant_symbols();
        auto sym_a = a.translate_symbol("a", 0);
        auto sym_b = a.translate_symbol("b", 0);
        auto sym_f = a.translate_symbol("f", 1);

        CHECK(consts.contains(sym_a));
        CHECK(consts.contains(sym_b));
        CHECK_FALSE(consts.contains(sym_f));
        CHECK(consts.size() == 2);
    }

    SECTION("get_constant_symbols empty when no constants") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("f", 1);
        a.translate_or_add_symbol("p", 2);

        CHECK(a.get_constant_symbols().empty());
    }

    SECTION("get_non_constant_symbols empty when only constants") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 0);
        a.translate_or_add_symbol("b", 0);

        CHECK(a.get_non_constant_symbols().empty());
    }

    SECTION("get_constant_symbols and get_non_constant_symbols partition the alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_symbol("a", 0);
        a.translate_or_add_symbol("b", 0);
        a.translate_or_add_symbol("f", 1);
        a.translate_or_add_symbol("p", 2);

        auto consts = a.get_constant_symbols();
        auto non_consts = a.get_non_constant_symbols();

        CHECK(consts.size() + non_consts.size() == a.get_number_of_symbols());

        // no overlap
        for (const Symbol s : consts) {
            CHECK(!non_consts.contains(s));
        }
    }
}
