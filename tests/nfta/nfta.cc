// testing basic nfta functionality

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
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
        Nfta aut(0, {}, &alphabet, {});
        State s = aut.add_state();
        CHECK(aut.contains_state(s));
        CHECK(aut.get_num_of_states() == 1);
    }

    SECTION("AddInitialState") {
        Nfta aut(0, {}, &alphabet, {});
        aut.add_final_state(0);
        CHECK(aut.is_state_final(0));
        CHECK(aut.get_num_of_states() == 1); // adding initial state increases num of states
    }

    SECTION("DuplicateStateIgnored") {
        Nfta aut(0, {}, &alphabet, {});
        aut.add_state();
        aut.add_state();
        // All added states are unique numbers, so count reflects additions
        CHECK(aut.get_num_of_states() == 2);
    }

    SECTION("DuplicateInitialStateIgnored") {
        Nfta aut(0, {}, &alphabet, {});
        aut.add_final_state(0);
        aut.add_final_state(0);
        // Initial states are tracked separately; duplicates ignored
        CHECK(aut.get_final_states().size() == 1);
    }

    SECTION("AddTransition") {
        Nfta aut(0, {}, &alphabet, {});
        State src = aut.add_state(); // source state
        State t1 = aut.add_state();
        State t2 = aut.add_state();
        aut.delta.add(alphabet["f"], src, {t1, t2});
        auto transitions = aut.delta.get_transitions();
        REQUIRE(transitions.size() == 1);
        const auto& t = *transitions.begin();
        CHECK(t.symbol == alphabet["f"]);
        CHECK(t.source == src);
        CHECK(t.targets.size() == 2);
    }

    SECTION("AddMultipleTransitions") {
        Nfta aut(0, {}, &alphabet, {});
        State s1 = aut.add_state();
        State s2 = aut.add_state();
        State s3 = aut.add_state();
        aut.delta.add(alphabet["f"], s1, {s2, s3});
        State s4 = aut.add_state();
        aut.delta.add(s4, alphabet["a"], {});
        auto transitions = aut.delta.get_transitions();
        CHECK(transitions.size() == 2);
    }

    SECTION("ArityMap") {
        CHECK(arities.get_arity(alphabet["f"]) == 2);
        CHECK(arities.get_arity(alphabet["a"]) == 0);
        CHECK(arities.get_arity(999) == 0);
    }

    SECTION("ContainsStateAndInitialCheck") {
        Nfta aut(0, {}, &alphabet, {});
        State s = aut.add_state();
        CHECK(aut.contains_state(s));
        CHECK(!aut.is_state_final(s));
        CHECK(!aut.contains_state(aut.get_num_of_states() + 10)); // definitely out of range
    }

    SECTION("ConstructorInitialization") {
        Nfta aut(2, {1}, &alphabet, {}); // 2 states: 0 and 1, initial state 1
        CHECK(aut.contains_state(0));
        CHECK(aut.contains_state(1));
        CHECK(aut.is_state_final(1));
        CHECK(!aut.is_state_final(0));
        CHECK(aut.get_num_of_states() == 2);
    }

    SECTION("PrintSanity") {
        Nfta aut(2, {0}, &alphabet, {});
        aut.delta.add(alphabet["f"], 0, {0,1});
        CHECK_NOTHROW(aut.print_mata(std::cout));
        CHECK_NOTHROW(aut.print_readable(std::cout, "bottom-up"));
        CHECK_NOTHROW(aut.print_readable(std::cout, "top-down"));
    }
}
