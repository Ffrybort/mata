#include <catch2/catch_test_macros.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

TEST_CASE("mata::nfta::is_empty") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a");
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("p");

    Symbol a = alphabet["a"];
    Symbol f = alphabet["f"];
    Symbol p = alphabet["p"];

    SECTION("Empty automaton") {
        Nfta aut;
        CHECK(aut.is_lang_empty());
    }

    SECTION("Empty delta") {
        Nfta aut({5}, &alphabet, Delta(0));
        CHECK(aut.is_lang_empty());
    }

    SECTION("No initial states") {
        Nfta aut({}, &alphabet, Delta(1));
        aut.delta.add(0, a, {});
        CHECK(aut.is_lang_empty());
    }

    SECTION("Single state accepts constant") {
        Nfta aut({0}, &alphabet, Delta(1));
        aut.delta.add(0, a, {});
        CHECK_FALSE(aut.is_lang_empty());
    }

    SECTION("Reachable leaf via unary symbol") {
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, f, {1});
        aut.delta.add(1, a, {});
        CHECK_FALSE(aut.is_lang_empty());
    }

    SECTION("No reachable leaf") {
        Nfta aut({0}, &alphabet, Delta(3));
        aut.delta.add(0, f, {1});
        aut.delta.add(1, f, {0});
        aut.delta.add(2, a, {});
        // no constant transition anywhere
        CHECK(aut.is_lang_empty());
    }

    SECTION("Reachable leaf via binary symbol") {
        // 0 -p-> (1, 1),  1 accepts a
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, p, {1, 1});
        aut.delta.add(1, a, {});
        CHECK_FALSE(aut.is_lang_empty());
    }

    SECTION("Binary symbol, no leaf") {
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, p, {1, 1});
        aut.delta.add(1, p, {0, 0});
        // no constant anywhere
        CHECK(aut.is_lang_empty());
    }

    SECTION("Multiple initial states, one reaches leaf") {
        Nfta aut({0, 1}, &alphabet, Delta(3));
        aut.delta.add(0, f, {2});
        aut.delta.add(1, a, {});
        CHECK_FALSE(aut.is_lang_empty());
    }

    SECTION("Multiple initial states, none reach leaf") {
        Nfta aut({0, 1}, &alphabet, Delta(5));
        aut.delta.add(0, f, {1});
        aut.delta.add(1, f, {0});
        aut.delta.add(2, a, {});
        aut.delta.add(3, a, {});
        aut.delta.add(4, a, {});
        CHECK(aut.is_lang_empty());
    }

    SECTION("Initial state is also a leaf state") {
        Nfta aut({0}, &alphabet, Delta(1));
        aut.delta.add(0, a, {});
        aut.delta.add(0, f, {0});
        CHECK_FALSE(aut.is_lang_empty());
    }
}