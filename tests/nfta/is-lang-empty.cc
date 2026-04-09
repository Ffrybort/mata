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
    alphabet.add_new_symbol("b");
    alphabet.add_new_symbol("g");

    Symbol a = alphabet["a"];
    Symbol f = alphabet["f"];
    Symbol p = alphabet["p"];
    Symbol b = alphabet["b"];
    Symbol g = alphabet["g"];

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

    SECTION("Large automaton — empty language") {
    Nfta aut({0}, &alphabet, Delta(8));

    // reachable cluster — no leaves
    aut.delta.add(0, f, {1});
    aut.delta.add(0, g, {2});
    aut.delta.add(0, p, {1, 2});
    aut.delta.add(0, p, {2, 1});
    aut.delta.add(0, p, {3, 3});

    aut.delta.add(1, f, {2});
    aut.delta.add(1, g, {3});
    aut.delta.add(1, p, {0, 2});
    aut.delta.add(1, p, {2, 0});
    aut.delta.add(1, p, {4, 4});

    aut.delta.add(2, f, {3});
    aut.delta.add(2, g, {0});
    aut.delta.add(2, p, {1, 3});
    aut.delta.add(2, p, {3, 1});
    aut.delta.add(2, p, {0, 4});

    aut.delta.add(3, f, {4});
    aut.delta.add(3, g, {1});
    aut.delta.add(3, p, {0, 1});
    aut.delta.add(3, p, {2, 4});
    aut.delta.add(3, p, {4, 0});

    aut.delta.add(4, f, {0});
    aut.delta.add(4, g, {2});
    aut.delta.add(4, p, {1, 4});
    aut.delta.add(4, p, {3, 0});
    aut.delta.add(4, p, {2, 2});

    aut.delta.add(5, a, {});
    aut.delta.add(5, b, {});
    aut.delta.add(5, f, {6});
    aut.delta.add(5, g, {7});
    aut.delta.add(5, p, {6, 7});

    aut.delta.add(6, a, {});
    aut.delta.add(6, f, {5});
    aut.delta.add(6, g, {7});
    aut.delta.add(6, p, {5, 7});
    aut.delta.add(6, p, {7, 5});

    aut.delta.add(7, b, {});
    aut.delta.add(7, f, {6});
    aut.delta.add(7, g, {5});
    aut.delta.add(7, p, {5, 5});
    aut.delta.add(7, p, {6, 6});

    CHECK(aut.is_lang_empty());
    }
}