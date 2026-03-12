#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata;
using namespace mata::utils;

TEST_CASE("mata::nfta::make_bottom_up_complete") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // constant
    alphabet.add_new_symbol("f"); // unary
    alphabet.add_new_symbol("g"); // binary
    alphabet.add_new_symbol("h"); // ternary

    SECTION("Default sink") {
        Nfta aut({}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {}); // repeated bottom-up left-hand side
        aut.delta.add(0, alphabet["f"], {0});
        aut.delta.add(1, alphabet["f"], {1});
        aut.delta.add(0, alphabet["g"], {0,1});
        aut.delta.add(1, alphabet["g"], {1,0});

        aut.make_bottom_up_complete({{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}});
        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 13 + 1);
    }

    SECTION("Custom sink") {
        Nfta aut({}, &alphabet, Delta(2));
        State sink = 4;

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {1});

        aut.make_bottom_up_complete({{alphabet["a"], 0}, {alphabet["f"], 1}, {alphabet["g"], 2}}, sink);
        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));

        // all states before sink should be added
        CHECK(aut.delta.num_of_states() == 5);
        CHECK(aut.delta.num_of_transitions() == 31);
    }

    SECTION("Empty delta") {
        Nfta aut({}, &alphabet, Delta(3));
        aut.make_bottom_up_complete({{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}});

        // Check that bottom-up complete
        CHECK(aut.is_bottom_up_complete(OrdVector<SymbolArity>{{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}}));
        CHECK(aut.delta.num_of_transitions() == 21);
    }

    SECTION("Nondeterministic delta with repeating symbols") {
        // repeated bottom-up left-hand side transition stay => increased final number of transitions
        Nfta aut({}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {}); // repeated
        aut.delta.add(0, alphabet["f"], {0});
        aut.delta.add(1, alphabet["f"], {1});
        aut.delta.add(0, alphabet["f"], {1}); // repeated
        aut.delta.add(0, alphabet["g"], {0,1});
        aut.delta.add(1, alphabet["g"], {1,0});
        aut.delta.add(0, alphabet["g"], {1,0}); // repeated

        aut.make_bottom_up_complete({{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}});
        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 13 + 3);
    }

    SECTION("Sink already exists") {
        Nfta aut({}, &alphabet, Delta(3));
        State sink = 0;

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});

        aut.make_bottom_up_complete({{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}}, sink);
        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 13 );
    }

    SECTION("Multiple states, constant symbol") {
        Nfta aut({}, &alphabet, Delta(5));
        // Only arity 0 symbol added
        aut.delta.add(0, alphabet["a"], {});
        aut.make_bottom_up_complete({{alphabet["a"],0}});

        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["a"]}));
        // nothing is added
        CHECK(aut.delta.num_of_transitions() == 1);
    }

    SECTION("High arity, partially empty delta") {
        Nfta aut({}, &alphabet, Delta(4));
        aut.delta.add(0, alphabet["h"], {0,1,2});
        aut.make_bottom_up_complete({{alphabet["h"],3}});

        CHECK(aut.is_bottom_up_complete(OrdVector<Symbol>{alphabet["h"]}));
        CHECK(aut.delta.num_of_states() == 5);
        CHECK(aut.delta.num_of_transitions() == 125);
    }
}

TEST_CASE("make_top_down_complete") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // constant
    alphabet.add_new_symbol("b"); // constant
    alphabet.add_new_symbol("f"); // unary
    alphabet.add_new_symbol("g"); // binary
    alphabet.add_new_symbol("h"); // ternary

    SECTION("Empty delta") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.make_top_down_complete({{alphabet["f"], 1}, {alphabet["g"], 2}});
        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["f"], alphabet["g"]}));
    }

    SECTION("Default sink") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["g"], {0,1});

        aut.make_top_down_complete({{alphabet["f"], 1}, {alphabet["g"], 2}});

        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["f"], alphabet["g"]}));
    }

    SECTION("Custom sink") {
        Nfta aut({0}, &alphabet, Delta(2));
        State sink = 9;

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {0});

        aut.make_top_down_complete({{alphabet["a"], 0}, {alphabet["f"], 1}, {alphabet["g"], 2}}, sink);

        CHECK_FALSE(aut.delta[sink].empty());
        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 21);
    }

    SECTION("Existing sink") {
        Nfta aut({0}, &alphabet, Delta(2));
        State sink = 0;

        aut.delta.add(0, alphabet["a"], {});

        aut.make_top_down_complete({{alphabet["a"], 0}, {alphabet["f"], 1}, {alphabet["g"], 2}}, sink);

        CHECK_FALSE(aut.delta[sink].empty());
        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 5);
    }

    SECTION("Only constant symbols") {
        // constants are ignored
        Nfta aut({0,1}, &alphabet, Delta(2));
        aut.delta.add(0, alphabet["a"], {});
        aut.make_top_down_complete({{alphabet["b"],0}});

        CHECK(aut.delta.num_of_transitions() == 1);
    }

    SECTION("Higher arity") {
        Nfta aut({0,1}, &alphabet, Delta(3));
        aut.delta.add(0, alphabet["h"], {0,1,0}); // only one existing transition
        aut.make_top_down_complete({{alphabet["h"],3}});

        CHECK(aut.is_top_down_complete(OrdVector<Symbol>{alphabet["h"]}));
        CHECK(aut.delta.num_of_states() == 4);
        CHECK(aut.delta.num_of_transitions() == 4);
    }
}
