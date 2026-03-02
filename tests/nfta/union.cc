#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

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

        B.add_final_state(0);

        A.union_nondet_in_place(B);

        CHECK(A.delta.num_of_states() == 1);
        CHECK(A.is_state_final(0));
    }

    SECTION("Final states are merged") {
        Nfta A({}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        A.add_final_state(0);
        B.add_final_state(1);

        A.union_nondet_in_place(B);

        CHECK(A.get_final_states().size() == 2);
        CHECK(A.is_state_final(0));
    }

    SECTION("Transitions from both automata are present") {
        Nfta A({0}, &alphabet, Delta(1));
        Nfta B({1}, &alphabet, Delta(2));

        A.delta.add(0, alphabet["a"], {0});
        B.delta.add(1, alphabet["f"], {4, 5});

        A.union_nondet_in_place(B);

        CHECK(A.delta.contains(0, alphabet["a"], {0}));
        CHECK(A.delta.contains(2, alphabet["f"], {5, 6}));
    }

    SECTION("Union increases number of states when necessary") {
        Nfta A({1}, &alphabet, Delta(1));
        Nfta B({1}, &alphabet, Delta(2));

        B.delta.add(1, alphabet["a"], {0});
        A.union_nondet_in_place(B);

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
        CHECK(C.is_state_final(4));
        CHECK(C.delta.contains(4, alphabet["f"], {5, 6, 7}));
    }

    SECTION("Union merges transitions") {
        Nfta A({0}, &alphabet, Delta(2));
        Nfta B({1}, &alphabet, Delta(2));

        A.delta.add(0, alphabet["f"], {1});
        B.delta.add(1, alphabet["f"], {1, 2});

        A.union_nondet_in_place(B);

        auto transitions = A.delta.get_transitions();

        size_t count = 0;
        for (const auto& t : transitions) {
            if (t.symbol == alphabet["f"])
                count++;
        }

        CHECK(count == 2);
    }
}
TEST_CASE("mata::nfta::union_product") {

    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("a");

    SECTION("No final states") {
        Nfta A({}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        Nfta C = union_product(A, B);

        CHECK(C.delta.num_of_states() == 0);
        CHECK(C.get_final_states().empty());
    }

     SECTION("Single-state, one side final") {
         Nfta A({0}, &alphabet, Delta(1));
         Nfta B({}, &alphabet, Delta(1));

         A.delta.add(0, alphabet["a"], {});

         Nfta C = union_product(A, B);

         CHECK(C.delta.num_of_states() == 1);
         CHECK(C.is_state_final(0));
     }

     SECTION("Identical automata") {
         Nfta A({0}, &alphabet, Delta(1));
         Nfta B({0}, &alphabet, Delta(1));

         A.delta.add(0, alphabet["a"], {});
         B.delta.add(0, alphabet["a"], {});

         Nfta C = union_product(A, B);

         CHECK(C.delta.num_of_states() == 1);
         CHECK(C.is_state_final(0));
     }


    SECTION("Union product – 3-state reachable product (contains-based)") {
        Nfta A({}, &alphabet, Delta(2));
        Nfta B({}, &alphabet, Delta(2));

        // A
        A.delta.add(0, alphabet["f"], {1});
        A.delta.add(1, alphabet["f"], {0});
        A.delta.add(0, alphabet["a"], {});
        A.delta.add(1, alphabet["a"], {});
        A.add_final_state(1);

        // B
        B.delta.add(0, alphabet["f"], {1});
        B.delta.add(1, alphabet["f"], {1});
        B.delta.add(0, alphabet["a"], {});
        B.delta.add(1, alphabet["a"], {});
        B.add_final_state(0);

        Nfta C = union_product(A, B);

        REQUIRE(C.delta.num_of_states() == 3);

        // State numbering by construction order:
        // 0 = (1,0)
        // 1 = (0,1)
        // 2 = (1,1)

        // --- Final states ---
        CHECK(C.is_state_final(0));        // (1,0)
        CHECK_FALSE(C.is_state_final(1));  // (0,1)
        CHECK(C.is_state_final(2));        // (1,1)

        // --- Transitions ---

        // (1,0) --f--> (0,1)
        CHECK(C.delta.contains(0, alphabet["f"], {1}));

        // (0,1) --f--> (1,1)
        CHECK(C.delta.contains(1, alphabet["f"], {2}));

        // (1,1) --f--> (0,1)
        CHECK(C.delta.contains(2, alphabet["f"], {1}));

        // "a" should produce empty tuple transitions
        CHECK(C.delta.contains(0, alphabet["a"], {}));
        CHECK(C.delta.contains(1, alphabet["a"], {}));
        CHECK(C.delta.contains(2, alphabet["a"], {}));
    }

}
