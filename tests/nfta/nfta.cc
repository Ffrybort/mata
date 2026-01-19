// testing basic nfta functionality

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/td/nfta.hh>
#include <mata/nfta/td/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

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
        Nfta aut(0, {}, Delta{}, &alphabet);
        State s = aut.add_state();
        CHECK(aut.contains_state(s));
        CHECK(aut.get_num_of_states() == 1);
    }

    SECTION("AddInitialState") {
        Nfta aut(0, {}, Delta{}, &alphabet);
        aut.add_initial_state(0);
        CHECK(aut.is_state_initial(0));
        CHECK(aut.get_num_of_states() == 0); // adding initial state doesn't increase num_of_states automatically
    }

    SECTION("DuplicateStateIgnored") {
        Nfta aut(0, {}, Delta{}, &alphabet);
        aut.add_state();
        aut.add_state();
        // All added states are unique numbers, so count reflects additions
        CHECK(aut.get_num_of_states() == 2);
    }

    SECTION("DuplicateInitialStateIgnored") {
        Nfta aut(0, {}, Delta{}, &alphabet);
        aut.add_initial_state(0);
        aut.add_initial_state(0);
        // Initial states are tracked separately; duplicates ignored
        CHECK(aut.get_initial_states().size() == 1);
    }

    SECTION("AddTransition") {
        Nfta aut(0, {}, Delta{}, &alphabet);
        State src = aut.add_state(); // source state
        State t1 = aut.add_state();
        State t2 = aut.add_state();
        aut.add_transition(alphabet["f"], src, {t1, t2});
        auto transitions = aut.get_transitions();
        REQUIRE(transitions.size() == 1);
        const auto& t = *transitions.begin();
        CHECK(t.symbol == alphabet["f"]);
        CHECK(t.source == src);
        CHECK(t.targets.size() == 2);
    }

    SECTION("AddMultipleTransitions") {
        Nfta aut(0, {}, Delta{}, &alphabet);
        State s1 = aut.add_state();
        State t1 = aut.add_state();
        State t2 = aut.add_state();
        aut.add_transition(alphabet["f"], s1, {t1, t2});
        State s2 = aut.add_state();
        aut.add_transition(alphabet["a"], s2, {});
        auto transitions = aut.get_transitions();
        CHECK(transitions.size() == 2);
    }

    SECTION("ArityMap") {
        CHECK(arities.get_arity(alphabet["f"]) == 2);
        CHECK(arities.get_arity(alphabet["a"]) == 0);
        CHECK(arities.get_arity(999) == 0);
    }

    SECTION("ContainsStateAndInitialCheck") {
        Nfta aut(0, {}, Delta{}, &alphabet);
        State s = aut.add_state();
        CHECK(aut.contains_state(s));
        CHECK(!aut.is_state_initial(s));
        CHECK(!aut.contains_state(aut.get_num_of_states() + 10)); // definitely out of range
    }

    SECTION("ConstructorInitialization") {
        Nfta aut(2, {1}, Delta{}, &alphabet); // 2 states: 0 and 1, initial state 1
        CHECK(aut.contains_state(0));
        CHECK(aut.contains_state(1));
        CHECK(aut.is_state_initial(1));
        CHECK(!aut.is_state_initial(0));
        CHECK(aut.get_num_of_states() == 2);
    }

    SECTION("PrintSanity") {
        Nfta aut(2, {0}, Delta{}, &alphabet);
        aut.add_transition(alphabet["f"], 0, {0,1});
        CHECK_NOTHROW(aut.print_mata(std::cout));
        CHECK_NOTHROW(aut.print_readable(std::cout));
    }
}
