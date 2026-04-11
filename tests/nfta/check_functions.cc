/**
 * Functions that check something - is-complete, is-deterministic,...
 */
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

TEST_CASE("mata::nfta determinism check") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("a");

    SECTION("Empty automaton") {
        Nfta aut({}, &alphabet, {});
        CHECK(aut.is_bottom_up_deterministic());
        CHECK(aut.is_top_down_deterministic());
    }

    SECTION("Deterministic both directions") {
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["a"], {});

        CHECK(aut.is_top_down_deterministic());
        CHECK(aut.is_bottom_up_deterministic());
    }

    SECTION("Bottom-up deterministic simple") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(0, alphabet["f"], {0});

        CHECK_FALSE(aut.is_top_down_deterministic());
        CHECK(aut.is_bottom_up_deterministic());
    }

    SECTION("Top down deterministic simple") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {1});

        CHECK(aut.is_top_down_deterministic());
        CHECK_FALSE(aut.is_bottom_up_deterministic());
    }

    SECTION("Nondeterministic both directions") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["f"],  {1});
        aut.delta.add(1, alphabet["f"], {1});
        aut.delta.add(0, alphabet["f"], {0});

        CHECK_FALSE(aut.is_top_down_deterministic());
        CHECK_FALSE(aut.is_bottom_up_deterministic());
    }

    SECTION("Top-down multiple initial") {
        Nfta aut({}, &alphabet, Delta(2));
        aut.add_initial_state(0);
        aut.add_initial_state(1);

        CHECK_FALSE(aut.is_top_down_deterministic());
    }

    SECTION("Bottom-up leaf nondeterminism") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        CHECK_FALSE(aut.is_bottom_up_deterministic());
        CHECK(aut.is_top_down_deterministic());
    }

    SECTION("Bottom-up deterministic with unary f and binary g") {
        alphabet.add_new_symbol("g"); // binary

        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});

        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(0, alphabet["f"], {1});

        aut.delta.add(0, alphabet["g"], {0,0});
        aut.delta.add(1, alphabet["g"], {0,1});
        aut.delta.add(1, alphabet["g"], {1,0});
        aut.delta.add(0, alphabet["g"], {1,1});

        CHECK(aut.is_bottom_up_deterministic());
        CHECK_FALSE(aut.is_top_down_deterministic());
    }

    SECTION("Bottom-up nondeterministic") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});

        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(0, alphabet["f"], {1});

        aut.delta.add(0, alphabet["g"], {0,0});
        aut.delta.add(1, alphabet["g"], {0,0});

        CHECK_FALSE(aut.is_bottom_up_deterministic());
        CHECK(aut.is_top_down_deterministic());
    }

    SECTION("Bottom-up deterministic three states") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});

        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(1, alphabet["f"], {1});
        aut.delta.add(2, alphabet["f"], {2});

        aut.delta.add(0, alphabet["g"], {0,0});
        aut.delta.add(1, alphabet["g"], {0,1});
        aut.delta.add(1, alphabet["g"], {1,0});
        aut.delta.add(1, alphabet["g"], {1,1});

        aut.delta.add(2, alphabet["g"], {2,0});
        aut.delta.add(2, alphabet["g"], {0,2});
        aut.delta.add(2, alphabet["g"], {2,1});
        aut.delta.add(2, alphabet["g"], {1,2});
        aut.delta.add(2, alphabet["g"], {2,2});

        CHECK(aut.is_bottom_up_deterministic());
        CHECK_FALSE(aut.is_top_down_deterministic());
    }

    SECTION("Big boy bottom-up deterministic") {
        alphabet.add_new_symbol("h"); // arity 4

        Nfta aut({0}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});

        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(2, alphabet["f"], {1});
        aut.delta.add(2, alphabet["f"], {2});
        aut.delta.add(4, alphabet["f"], {3});
        aut.delta.add(4, alphabet["f"], {4});

        aut.delta.add(3, alphabet["g"], {0,0});
        aut.delta.add(3, alphabet["g"], {1,0});
        aut.delta.add(3, alphabet["g"], {0,1});
        aut.delta.add(3, alphabet["g"], {1,1});

        aut.delta.add(2, alphabet["g"], {2,0});
        aut.delta.add(2, alphabet["g"], {0,2});
        aut.delta.add(2, alphabet["g"], {2,1});
        aut.delta.add(2, alphabet["g"], {1,2});
        aut.delta.add(2, alphabet["g"], {2,2});

        aut.delta.add(4, alphabet["g"], {3,3});
        aut.delta.add(4, alphabet["g"], {4,0});
        aut.delta.add(4, alphabet["g"], {0,4});
        aut.delta.add(4, alphabet["g"], {4,4});
        aut.delta.add(4, alphabet["g"], {3,4});
        aut.delta.add(4, alphabet["g"], {4,3});

        aut.delta.add(0, alphabet["h"], {0,0,0,0});
        aut.delta.add(1, alphabet["h"], {1,1,1,1});
        aut.delta.add(2, alphabet["h"], {2,2,2,2});
        aut.delta.add(3, alphabet["h"], {3,3,3,3});
        aut.delta.add(4, alphabet["h"], {4,4,4,4});

        aut.delta.add(4, alphabet["h"], {0,0,0,1});
        aut.delta.add(4, alphabet["h"], {0,0,1,0});
        aut.delta.add(4, alphabet["h"], {0,1,0,0});
        aut.delta.add(4, alphabet["h"], {1,0,0,0});
        aut.delta.add(4, alphabet["h"], {2,2,2,3});
        aut.delta.add(4, alphabet["h"], {3,2,2,2});
        aut.delta.add(4, alphabet["h"], {1,2,1,2});
        aut.delta.add(4, alphabet["h"], {0,3,0,3});
        aut.delta.add(4, alphabet["h"], {4,1,4,1});
        aut.delta.add(4, alphabet["h"], {2,3,4,0});

        CHECK(aut.is_bottom_up_deterministic());
        CHECK_FALSE(aut.is_top_down_deterministic());
    }

    SECTION("Big top-down deterministic (not b-u) monster") {
        Nfta aut({0}, &alphabet, Delta(4));

        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(3, alphabet["a"], {});

        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(3, alphabet["f"], {3});
        aut.delta.add(2, alphabet["f"], {2});

        aut.delta.add(0, alphabet["g"], {1,1});
        aut.delta.add(1, alphabet["g"], {0,0});
        aut.delta.add(2, alphabet["g"], {2,2});
        aut.delta.add(3, alphabet["g"], {3,3});

        aut.delta.add(0, alphabet["h"], {2,2,2,2});
        aut.delta.add(1, alphabet["h"], {2,2,2,2});
        aut.delta.add(2, alphabet["h"], {3,3,3,3});
        aut.delta.add(3, alphabet["h"], {0,0,0,0});

        CHECK(aut.is_top_down_deterministic());
        CHECK_FALSE(aut.is_bottom_up_deterministic());
    }
}

TEST_CASE("mata::nfta completeness check") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f"); // binary
    alphabet.add_new_symbol("g"); // unary
    alphabet.add_new_symbol("a"); // constant

    SECTION("Bottom-up complete automaton") {
        Nfta aut({}, &alphabet, Delta{2});

        aut.delta.add( 0, alphabet["a"], {});

        aut.delta.add( 0, alphabet["g"], {0});
        aut.delta.add( 0, alphabet["g"], {1});
        aut.delta.add( 1, alphabet["g"], {0});
        aut.delta.add( 1, alphabet["g"], {1});

        aut.delta.add(0, alphabet["f"],  {0,0});
        aut.delta.add(0, alphabet["f"],  {0,1});
        aut.delta.add(0, alphabet["f"],  {1,0});
        aut.delta.add(0, alphabet["f"],  {1,1});
        aut.delta.add(1, alphabet["f"],  {0,0});
        aut.delta.add(1, alphabet["f"],  {0,1});
        aut.delta.add(1, alphabet["f"],  {1,0});
        aut.delta.add(1, alphabet["f"],  {1,1});

        CHECK(aut.is_bottom_up_complete(alphabet.get_alphabet_symbols()));
        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["g"], alphabet["f"]}));
    }

    SECTION("Bottom-up incomplete automaton") {
        Nfta aut({}, &alphabet, {});

        aut.delta.add(0, alphabet["a"],  {});
        aut.delta.add(1, alphabet["a"],  {});

        aut.delta.add(0, alphabet["g"], {0});
        aut.delta.add(1, alphabet["g"], {0});

        CHECK_FALSE(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["g"]}));
        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["g"]}));

        aut.delta.add(0, alphabet["g"], {1});
        // now complete without f
        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["g"]}));
    }

    SECTION("Top-down complete") {
        Nfta aut({}, &alphabet, {});

        State s0 = aut.delta.add_state();
        State s1 = aut.delta.add_state();
        aut.add_initial_state(s0);
        aut.add_initial_state(s1);

        // constants
        aut.delta.add(s0, alphabet["a"], {});
        aut.delta.add(s1, alphabet["a"], {});

        // unary "g"
        aut.delta.add(s0, alphabet["g"], {s0});
        aut.delta.add(s1, alphabet["g"], {s1});

        // binary "f"
        aut.delta.add(s0, alphabet["f"], {s0,s0});
        aut.delta.add(s0, alphabet["f"], {s1,s1});
        aut.delta.add(s1, alphabet["f"], {s0,s0});
        aut.delta.add(s1, alphabet["f"], {s1,s1});

        CHECK(aut.is_top_down_complete(alphabet.get_alphabet_symbols()));
    }

    SECTION("Top-down incomplete") {
        Nfta aut({0}, &alphabet, {});

        aut.add_initial_state(0);

        // constant
        aut.delta.add(0, alphabet["a"], {});

        // binary "f" partially defined
        aut.delta.add(0, alphabet["f"], {0,0});
        // missing other combinations

        CHECK_FALSE(aut.is_top_down_complete(alphabet.get_alphabet_symbols()));
    }

    SECTION("Incomplete both directions (higher arities)") {
        alphabet.add_new_symbol("h"); // arity 3
        alphabet.add_new_symbol("k"); // arity 4

        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(0, alphabet["g"], {0});
        aut.delta.add(1, alphabet["g"], {1});

        // only a few tuples
        aut.delta.add(0, alphabet["h"], {0,0,0});
        aut.delta.add(1, alphabet["h"], {1,1,1});

        aut.delta.add(2, alphabet["k"], {0,0,0,0});

        CHECK_FALSE(aut.is_bottom_up_complete(alphabet.get_alphabet_symbols()));
        CHECK_FALSE(aut.is_top_down_complete(alphabet.get_alphabet_symbols()));
    }

    SECTION("Bottom-up complete but not top-down complete") {
        Nfta aut({}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});

        for (State s : {0u,1u}) {
            aut.delta.add(0, alphabet["g"], {s});
        }

        // all tuples for h
        for (State a : {0u,1u}) {
            for (State b : {0u,1u}) {
                for (State c : {0u,1u}) {
                    aut.delta.add(0, alphabet["h"], {a,b,c});
                    aut.delta.add(1, alphabet["h"], {a,b,c});
                }
            }
        }

        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["g"], alphabet["h"]}));
        CHECK_FALSE(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["g"], alphabet["h"]}));
    }

    SECTION("Top-down complete but not bottom-up complete") {
        Nfta aut({}, &alphabet, Delta(2));

        aut.add_initial_state(0);
        aut.add_initial_state(1);

        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(0, alphabet["g"], {0});
        aut.delta.add(1, alphabet["g"], {1});

        aut.delta.add(0, alphabet["h"], {0,0,0});
        aut.delta.add(1, alphabet["h"], {1,1,1});

        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["g"], alphabet["h"]}));
        CHECK_FALSE(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["g"], alphabet["h"]}));
    }

    SECTION("Complete both directions with arity 4 symbol") {
        alphabet.add_new_symbol("h"); // arity 3
        alphabet.add_new_symbol("k"); // arity 4

        Nfta aut({}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});

        for (State s : {0u,1u}) {
            aut.delta.add(0, alphabet["g"], {s});
            aut.delta.add(1, alphabet["g"], {s});
        }

        // all tuples for h
        for (State a : {0u,1u}) {
            for (State b : {0u,1u}) {
                for (State c : {0u,1u}) {
                    aut.delta.add(0, alphabet["h"], {a,b,c});
                    aut.delta.add(1, alphabet["h"], {a,b,c});
                }
            }
        }

        // all tuples for k
        for (State a : {0u,1u}) {
            for (State b : {0u,1u}) {
                for (State c : {0u,1u}) {
                    for (State d : {0u,1u}) {
                        aut.delta.add(0, alphabet["k"], {a,b,c,d});
                        aut.delta.add(1, alphabet["k"], {a,b,c,d});
                    }
                }
            }
        }

        CHECK(aut.is_bottom_up_complete(aut.delta.get_used_symbols(false)));
        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["g"], alphabet["h"], alphabet["k"]}));
    }
}


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

    SECTION("Tricky automaton") {
        Nfta aut({0}, &alphabet, Delta(3));
        aut.delta.add(0, f, {1, 2});
        aut.delta.add(1, a, {});
        auto reachable = aut.get_bottom_up_reachable();
        CHECK(reachable[1]);
        CHECK_FALSE(reachable[0]);
        CHECK_FALSE(reachable[2]);
        CHECK(aut.is_lang_empty());
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