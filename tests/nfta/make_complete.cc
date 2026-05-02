#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>
#include <mata/nfa/types.hh>

using namespace mata::nfta;
using namespace mata;
using namespace mata::utils;

TEST_CASE("mata::nfta::make_complete") {
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

        OrdVector<SymbolArity> symbols = {{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}};
        aut.make_complete(&symbols);
        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 14);
    }

    SECTION("Custom sink") {
        Nfta aut({}, &alphabet, Delta(2));
        State sink = 4;

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {1});

        OrdVector<SymbolArity> symbols = {{alphabet["a"], 0}, {alphabet["f"], 1}, {alphabet["g"], 2}};
        aut.make_complete(&symbols, sink);
        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));

        // all states before sink should be added
        CHECK(aut.delta.num_of_states() == 5);
        CHECK(aut.delta.num_of_transitions() == 31);
    }

    SECTION("Empty delta") {
        Nfta aut({}, &alphabet, Delta(3));
        OrdVector<SymbolArity> symbols = {{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}};
        aut.make_complete(&symbols);

        // Check that bottom-up complete
        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
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

        OrdVector<SymbolArity> symbols = {{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}};
        aut.make_complete();
        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 13 + 3);
    }

    SECTION("Sink already exists") {
        Nfta aut({}, &alphabet, Delta(3));
        State sink = 0;

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});

        OrdVector<SymbolArity> symbols = {{alphabet["a"],0}, {alphabet["f"],1}, {alphabet["g"],2}};
        aut.make_complete(&symbols, sink);
        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["a"], alphabet["f"], alphabet["g"]}));
        CHECK(aut.delta.num_of_transitions() == 13 );
    }

    SECTION("Multiple states, constant symbol") {
        Nfta aut({}, &alphabet, Delta(5));
        // Only arity 0 symbol added
        aut.delta.add(0, alphabet["a"], {});
        OrdVector<SymbolArity> symbols = {{alphabet["a"],0}};
        aut.make_complete(&symbols);

        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["a"]}));
        // nothing is added
        CHECK(aut.delta.num_of_transitions() == 1);
    }

    SECTION("High arity, partially empty delta") {
        Nfta aut({}, &alphabet, Delta(4));
        aut.delta.add(0, alphabet["h"], {0,1,2});
        OrdVector<SymbolArity> symbols = {{alphabet["h"],3}};
        aut.make_complete(&symbols);

        CHECK(aut.is_complete(OrdVector<Symbol>{alphabet["h"]}));
        CHECK(aut.delta.num_of_states() == 5);
        CHECK(aut.delta.num_of_transitions() == 125);
    }
}
