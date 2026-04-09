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

TEST_CASE("mata::nfta") {
    OnTheFlyAlphabet alphabet;
    // populate the alphabet
    alphabet.add_new_symbol("f"); // function symbol
    alphabet.add_new_symbol("a"); // constant symbol

    SECTION("Add initial") {
        Nfta aut({}, &alphabet, {});
        aut.add_initial_state(0);
        CHECK(aut.is_state_initial(0));
        CHECK(aut.delta.num_of_states() == 1); // adding initial state increases num of states
    }

    SECTION("Duplicate initial") {
        Nfta aut({}, &alphabet, {});
        aut.add_initial_state(0);
        aut.add_initial_state(0);
        // Initial states are tracked separately; duplicates ignored
        CHECK(aut.get_initial_states().size() == 1);
    }


    SECTION("Contains state and initial state") {
        Nfta aut({}, &alphabet, {});
        State s = aut.delta.add_state();
        CHECK(aut.delta.contains_state(s));
        CHECK(!aut.is_state_initial(s));
        CHECK(!aut.delta.contains_state(static_cast<State>(aut.delta.num_of_states()))); // should out of range
    }

    SECTION("Constructor initialization") {
        Nfta aut( {1}, &alphabet, Delta(2)); // 2 states: 0 and 1, initial state 1
        CHECK(aut.delta.contains_state(0));
        CHECK(aut.delta.contains_state(1));
        CHECK(aut.is_state_initial(1));
        CHECK(!aut.is_state_initial(0));
        CHECK(aut.delta.num_of_states() == 2);
    }

    SECTION("Print sanity") {
        Nfta aut({0}, &alphabet, {});
        aut.delta.add(0, alphabet["f"], {0,1});
        CHECK_NOTHROW(aut.print_mata(std::cout));
        CHECK_NOTHROW(aut.print_readable(std::cout));
        CHECK_NOTHROW(aut.print_timbuk(std::cout));
    }

    SECTION("Swap initial") {
        Nfta aut({0, 3, 6, 9}, &alphabet, {Delta(10)});
        aut.swap_initial_states();
        CHECK(!aut.is_state_initial(0));
        CHECK(aut.is_state_initial(1));
        CHECK(aut.is_state_initial(2));
        CHECK(!aut.is_state_initial(3));
        CHECK(aut.is_state_initial(4));
        CHECK(aut.is_state_initial(5));
        CHECK(!aut.is_state_initial(6));
        CHECK(aut.is_state_initial(7));
        CHECK(aut.is_state_initial(8));
        CHECK(!aut.is_state_initial(9));
    }

    SECTION("Defragment basic") {
        Nfta aut({}, &alphabet, {});
        State s0 = aut.delta.add_state();
        State s1 = aut.delta.add_state();
        State s2 = aut.delta.add_state();

        aut.delta.add(s0, alphabet["f"], {s1, s2});
        aut.delta.add(s1, alphabet["f"], {s2});
        aut.delta.add(s2, alphabet["a"], {s0});

        aut.add_initial_state(2);

        BoolVector is_staying{true, false, true};
        aut.defragment(is_staying);

        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.contains(0, alphabet["f"], {1,2}) == false);
        CHECK(aut.is_state_initial(1));
    }

    SECTION("DefragmentAllStatesRemoved") {
        Nfta aut({}, &alphabet, {});

        aut.delta.add_state();
        aut.delta.add_state();
        aut.add_initial_state(1);

        BoolVector is_staying{false, false};
        aut.defragment(is_staying);

        CHECK(aut.delta.num_of_states() == 0);
        CHECK(aut.get_initial_states().empty());
    }

    SECTION("DefragmentIdentityCase") {
        Nfta aut({}, &alphabet, {});

        State s0 = aut.delta.add_state();
        State s1 = aut.delta.add_state();

        aut.delta.add(s0, alphabet["f"], {s1});

        BoolVector is_staying{true, true};
        aut.defragment(is_staying);

        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.contains(0, alphabet["f"], {1}));
    }
}
