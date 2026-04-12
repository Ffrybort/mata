#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>
#include <mata/nfta/builder.hh>

using namespace mata::nfta;
using namespace mata;

TEST_CASE("mata::nfta::union_nondet") {

    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("a");

    SECTION("Union of disjoint single-state automata") {
        Nfta A({}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        B.add_initial_state(0);

        A.unite_nondet_with(B);

        CHECK(A.delta.num_of_states() == 1);
        CHECK(A.is_state_initial(0));
    }

    SECTION("Final states are merged") {
        Nfta A({}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        A.add_initial_state(0);
        B.add_initial_state(1);

        A.unite_nondet_with(B);

        CHECK(A.get_initial_states().size() == 2);
        CHECK(A.is_state_initial(0));
    }

    SECTION("Transitions from both automata are present") {
        Nfta A({0}, &alphabet, Delta(1));
        Nfta B({1}, &alphabet, Delta(2));

        A.delta.add(0, alphabet["a"], {0});
        B.delta.add(1, alphabet["f"], {4, 5});

        A.unite_nondet_with(B);

        CHECK(A.delta.contains(0, alphabet["a"], {0}));
        CHECK(A.delta.contains(2, alphabet["f"], {5, 6}));
    }

    SECTION("Union increases number of states when necessary") {
        Nfta A({1}, &alphabet, Delta(1));
        Nfta B({1}, &alphabet, Delta(2));

        B.delta.add(1, alphabet["a"], {0});
        A.unite_nondet_with(B);

        CHECK(A.delta.num_of_states() == 3);
    }

    SECTION("union_nondet returns new automaton") {
        Nfta A({0}, &alphabet, Delta(1));
        Nfta B({0}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {1, 2, 3});
        B.delta.add(0, alphabet["f"], {1, 2, 3});

        Nfta C = union_nondet(A, B);

        CHECK(A.delta.num_of_transitions() == 1);

        CHECK(C.delta.contains(0, alphabet["a"], {1, 2, 3}));
        // B got renumbered
        CHECK(C.is_state_initial(4));
        CHECK(C.delta.contains(4, alphabet["f"], {5, 6, 7}));
    }

    SECTION("Union merges transitions") {
        Nfta A({0}, &alphabet, Delta(2));
        Nfta B({1}, &alphabet, Delta(2));

        A.delta.add(0, alphabet["f"], {1});
        B.delta.add(1, alphabet["f"], {1, 2});

        A.unite_nondet_with(B);

        auto transitions = A.delta.get_transitions();

        size_t count = 0;
        for (const auto& t : transitions) {
            if (t.symbol == alphabet["f"])
                count++;
        }

        CHECK(count == 2);
    }
}

TEST_CASE("mata::nfta::union_det") {

    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("a");

    SECTION("No final states") {
        Nfta A({}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        Nfta C = union_det(A, B);

        CHECK(C.is_identical_to(create_empty()));
    }

     SECTION("Single-state, one side final") {
         Nfta A({0}, &alphabet, Delta(1));
         Nfta B({}, &alphabet, Delta(1));

         A.delta.add(0, alphabet["a"], {});
         B.delta.add(0, alphabet["a"], {});

         Nfta C = union_det(A, B);

         CHECK(C.delta.num_of_states() == 1);
         CHECK(C.initial_states.size() == 1);
         CHECK(C.delta.contains(0, alphabet["a"], {}));
     }

    SECTION("Identical automata") {
        Nfta A({0}, &alphabet, Delta(1));
        Nfta B({0}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        Nfta C = union_det(A, B);

        CHECK(C.delta.num_of_states() == 1);
        CHECK(C.initial_states.size() == 1);
        CHECK(C.delta.contains(0, alphabet["a"], {}));
    }

    SECTION("Union product – 3-state") {
        Nfta A({1}, &alphabet, Delta(2));
        Nfta B({0}, &alphabet, Delta(3));

        A.delta.add(0, alphabet["a"], {});
        A.delta.add(0, alphabet["f"], {1});
        A.delta.add(1, alphabet["f"], {0});

        B.delta.add(2, alphabet["f"], {0});
        B.delta.add(0, alphabet["f"], {1});
        B.delta.add(1, alphabet["f"], {2});
        B.delta.add(2, alphabet["f"], {2});
        B.delta.add(1, alphabet["a"], {});

        utils::TwoDimensionalMap<State> map(A.delta.num_of_states(), B.delta.num_of_states());
        Nfta C = union_det(A, B, &map);

        REQUIRE(C.delta.num_of_states() == 6);

        State s00 = map.get(0, 0);
        State s01 = map.get(0, 1);
        State s02 = map.get(0, 2);
        State s10 = map.get(1, 0);
        State s11 = map.get(1, 1);
        State s12 = map.get(1, 2);
        CHECK(C.initial_states.size() == 4);
        CHECK(C.is_state_initial(s10));
        CHECK(C.is_state_initial(s11));
        CHECK(C.is_state_initial(s12));
        CHECK(C.is_state_initial(s00));

        CHECK(C.delta.contains(s01, alphabet["a"], {}));
        CHECK(C.delta.contains(s10, alphabet["f"], {s01}));
        CHECK(C.delta.contains(s02, alphabet["f"], {s10}));
        CHECK(C.delta.contains(s11, alphabet["f"], {s02}));
        CHECK(C.delta.contains(s12, alphabet["f"], {s02}));
        CHECK(C.delta.contains(s00, alphabet["f"], {s11}));
        CHECK(C.delta.contains(s01, alphabet["f"], {s12}));
        CHECK(C.delta.contains(s02, alphabet["f"], {s12}));
        CHECK(C.delta.contains(s12, alphabet["f"], {s00}));
        CHECK(C.delta.num_of_transitions() == 9);
    }

    SECTION("Union leaves only") {
        alphabet.add_new_symbol("b");

        Nfta A({0}, &alphabet, Delta(2));
        Nfta B({0}, &alphabet, Delta(2));

        A.delta.add(0, alphabet["a"], {});
        A.delta.add(1, alphabet["b"], {});

        B.delta.add(0, alphabet["b"], {});
        B.delta.add(1, alphabet["a"], {});

        // L(A) = {a}, L(B) = {b}
        // L(C) = {a, b}

        Nfta C = union_det(A, B);
        // CHECK(C.initial_states.size() == 2);
        // CHECK(C.delta.num_of_transitions() == 2);
    }
}
