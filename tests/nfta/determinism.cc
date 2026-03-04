#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

TEST_CASE("mata::nfta determinism") {
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
        alphabet.add_new_symbol_res("h"); // arity 4

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