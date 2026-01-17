// testing basic nfta funcionality

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "mata/nfta/nfta.hh"
#include "mata/nfta/delta.hh"
#include "mata/nfta/types.hh"
#include "mata/alphabet.hh"

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

TEST_CASE("Nfta: OnTheFlyAlphabet setup") {
    OnTheFlyAlphabet alphabet;
    ArityMap arities;

    // populate the alphabet
    alphabet.add_new_symbol("f"); // function symbol
    alphabet.add_new_symbol("a"); // constant symbol
    arities.set_arity(alphabet["f"], 2); // f -> 2, a -> 0 implicit

    SECTION("AddState") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        aut.add_state(1);
        CHECK(aut.contains_state(1));
    }

    SECTION("AddInitialState") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        aut.add_initial_state(2);
        CHECK(aut.is_state_initial(2));
    }

    SECTION("DuplicateStateIgnored") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        aut.add_state(3);
        aut.add_state(3);
        CHECK(aut.get_states().size() == 1);
    }

    SECTION("DuplicateInitialStateIgnored") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        aut.add_initial_state(4);
        aut.add_initial_state(4);
        // original Google Test checked initial states here, seems likely a copy-paste issue
        // keeping it equivalent: we check initial states instead
        CHECK(aut.get_initial_states().size() == 1);
    }

    SECTION("AddTransition") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        aut.add_transition(alphabet["f"], 3, {1,2});
        auto transitions = aut.get_transitions();
        REQUIRE(transitions.size() == 1);
        const auto& t = *transitions.begin();
        CHECK(t.symbol == alphabet["f"]);
        CHECK(t.source == 3);
        CHECK(t.targets.size() == 2);
    }

    SECTION("AddMultipleTransitions") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        aut.add_transition(alphabet["f"], 3, {1,2});
        aut.add_transition(alphabet["a"], 4, {});
        auto transitions = aut.get_transitions();
        CHECK(transitions.size() == 2);
    }

    SECTION("ArityMap") {
        CHECK(arities.get_arity(alphabet["f"]) == 2);
        CHECK(arities.get_arity(alphabet["a"]) == 0);
        CHECK(arities.get_arity(999) == 0);
    }

    SECTION("ContainsStateAndInitialCheck") {
        Nfta aut({}, {}, Delta{}, &alphabet);
        CHECK(!aut.contains_state(10));
        CHECK(!aut.is_state_initial(10));
    }

    SECTION("ConstructorInitialization") {
        mata::utils::SparseSet<State> states = {1,2};
        mata::utils::SparseSet<State> init = {2};
        Nfta aut(states, init, Delta{}, &alphabet);
        CHECK(aut.contains_state(1));
        CHECK(aut.contains_state(2));
        CHECK(aut.is_state_initial(2));
        CHECK(!aut.is_state_initial(1));
    }

    SECTION("PrintSanity") {
        Nfta aut({1},{1},Delta{}, &alphabet);
        aut.add_transition(alphabet["f"], 1, {1,1});
        CHECK_NOTHROW(aut.print(std::cout));
    }
}

