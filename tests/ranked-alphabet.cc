#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "mata/alphabet.hh"

using namespace mata;
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
        Symbol s = a.translate_or_add_ranked_symbol("a", 2);
        CHECK(s == 0);
        CHECK(a.get_number_of_symbols() == 1);
        CHECK(a.get_next_value() == 1);
    }

    SECTION("translate_or_add_ranked_symbol ignores duplicits") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_ranked_symbol("a", 2);
        Symbol s2 = a.translate_or_add_ranked_symbol("a", 2);
        CHECK(s1 == s2);
        CHECK(a.get_number_of_symbols() == 1);
    }

    SECTION("same name different arity yields different symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_ranked_symbol("a", 1);
        Symbol s2 = a.translate_or_add_ranked_symbol("a", 2);
        CHECK(s1 != s2);
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("translate_ranked_symbol throws if missing") {
        RankedOnTheFlyAlphabet a{};
        CHECK_THROWS(a.translate_ranked_symbol("missing", 0));
    }

    SECTION("translate_ranked_symbol returns existing symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_ranked_symbol("a", 3);
        CHECK(a.translate_ranked_symbol("a", 3) == s);
    }

    SECTION("initializer_list constructor (StringArity -> Symbol)") {
        RankedOnTheFlyAlphabet a{
            { {{"a", 1}, 5}, {{"b", 2}, 7} }
        };
        CHECK(a.translate_ranked_symbol("a", 1) == 5);
        CHECK(a.translate_ranked_symbol("b", 2) == 7);
        CHECK(a.get_next_value() == 8);
    }

    SECTION("vector<StringArity> constructor") {
        std::vector<RankedOnTheFlyAlphabet::StringArity> v{
            {"a", 1}, {"b", 2}
        };
        RankedOnTheFlyAlphabet a{v};
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("iterator constructor") {
        std::vector<RankedOnTheFlyAlphabet::StringArity> v{
            {"x", 0}, {"y", 1}
        };
        RankedOnTheFlyAlphabet a{v.begin(), v.end()};
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("get_alphabet_symbols_arities returns ordered vector") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("b", 1);
        a.translate_or_add_ranked_symbol("a", 2);
        auto syms = a.get_alphabet_symbols_arities();
        CHECK(syms.size() == 2);
    }

    SECTION("get_alphabet_symbols returns symbols only") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("a", 1);
        a.translate_or_add_ranked_symbol("b", 2);
        auto syms = a.get_alphabet_symbols();
        CHECK(syms.size() == 2);
    }

    SECTION("reverse_translate_symbol works") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_ranked_symbol("a", 1);
        CHECK(a.reverse_translate_symbol(s) == "a");
    }

    SECTION("get_arity returns correct arity") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_ranked_symbol("a", 4);
        CHECK(a.get_arity(s) == 4);
    }

    SECTION("set_arity updates arity") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_ranked_symbol("a", 1);
        a.set_arity(s, 3);
        CHECK(a.get_arity(s) == 3);
    }

    SECTION("erase by symbol removes entry") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_ranked_symbol("a", 1);
        CHECK(a.erase(s) == 1);
        CHECK(a.empty());
    }

    SECTION("clear resets alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("a", 1);
        a.translate_or_add_ranked_symbol("b", 2);
        a.clear();
        CHECK(a.empty());
        CHECK(a.get_number_of_symbols() == 0);
        CHECK(a.get_next_value() == 0);
    }

     SECTION("translate_or_add_ranked_symb adds and reuses symbol") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_ranked_symbol("a", 2);
        Symbol s2 = a.translate_or_add_ranked_symbol("a", 2);
        CHECK(s1 == s2);
        CHECK(a.get_number_of_symbols() == 1);
    }

    SECTION("translate_or_add_symb uses implicit arity 0") {
        RankedOnTheFlyAlphabet a{};
        Symbol s = a.translate_or_add_symb("x");
        CHECK(a.translate_ranked_symbol("x", 0) == s);
    }

    SECTION("translate_symb throws if missing") {
        RankedOnTheFlyAlphabet a{};
        CHECK_THROWS(a.translate_symb("missing"));
    }

    SECTION("get_complement returns missing symbols") {
        RankedOnTheFlyAlphabet a{};
        Symbol s1 = a.translate_or_add_ranked_symbol("a", 0);
        Symbol s2 = a.translate_or_add_ranked_symbol("b", 0);

        utils::OrdVector<Symbol> subset{ s1 };
        auto complement = a.get_complement(subset);

        CHECK(complement.size() == 1);
        CHECK(*complement.begin() == s2);
    }


    SECTION("add_symbols_from(vector<StringArity>) inserts all symbols") {
        RankedOnTheFlyAlphabet a{};
        std::vector<RankedOnTheFlyAlphabet::StringArity> symbols{
            {"a", 1}, {"b", 2}
        };
        a.add_symbols_from(symbols);
        CHECK(a.get_number_of_symbols() == 2);
    }

    SECTION("add_symbols_from(SymbolMap) does not overwrite existing symbols") {
        RankedOnTheFlyAlphabet a{};
        Symbol existing = a.translate_or_add_ranked_symbol("a", 1);

        RankedOnTheFlyAlphabet::SymbolMap map{
            {{"a", 1}, 42},
            {{"b", 2}, 7}
        };

        a.add_symbols_from(map);

        CHECK(a.translate_ranked_symbol("a", 1) == existing);
        CHECK(a.translate_ranked_symbol("b", 2) == 7);
    }

    SECTION("add_new_symbol throws on duplicate") {
        RankedOnTheFlyAlphabet a{};
        a.add_new_symbol("a", 1);
        CHECK_THROWS(a.add_new_symbol("a", 1));
    }

    SECTION("string + arity constructor throws on size mismatch") {
        std::vector<std::string> names{ "a", "b" };
        std::vector<unsigned> arities{ 1 };
        CHECK_THROWS(RankedOnTheFlyAlphabet{ names, arities });
    }

    SECTION("erase by StringArity removes symbol") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("a", 1);
        CHECK(a.erase(RankedOnTheFlyAlphabet::StringArity{"a", 1}) == 1);
        CHECK(a.empty());
    }

    SECTION("erase by name and arity removes symbol") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("a", 1);
        CHECK(a.erase("a", 1) == 1);
        CHECK(a.empty());
    }

    SECTION("copy constructor copies alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("a", 1);

        RankedOnTheFlyAlphabet b{ a };
        CHECK(b.get_number_of_symbols() == 1);
        CHECK(b.translate_ranked_symbol("a", 1) ==
              a.translate_ranked_symbol("a", 1));
    }

    SECTION("move constructor transfers alphabet") {
        RankedOnTheFlyAlphabet a{};
        a.translate_or_add_ranked_symbol("a", 1);

        RankedOnTheFlyAlphabet b{ std::move(a) };
        CHECK(b.get_number_of_symbols() == 1);
    }
}
