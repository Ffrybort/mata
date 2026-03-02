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
// TEST_CASE("mata::nfta::union_product") {
//
//     OnTheFlyAlphabet alphabet;
//     alphabet.add_new_symbol("f");
//     alphabet.add_new_symbol("a");
//
//     SECTION("Single-state, none final") {
//         Nfta A({}, &alphabet, Delta(1));
//         Nfta B({}, &alphabet, Delta(1));
//
//         A.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 0, {});
//
//         Nfta C = union_product(A, B);
//
//         CHECK(C.delta.num_of_states() == 1);
//         CHECK(C.get_final_states().empty());
//     }
//
//     SECTION("Single-state, one side final") {
//         Nfta A({}, &alphabet, Delta(1));
//         Nfta B({}, &alphabet, Delta(1));
//
//         A.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 0, {});
//
//         A.add_final_state(0);
//
//         Nfta C = union_product(A, B);
//
//         CHECK(C.delta.num_of_states() == 1);
//         CHECK(C.is_state_final(0));
//     }
//
//     SECTION("Single-state, both final") {
//         Nfta A({0}, &alphabet, Delta(1));
//         Nfta B({0}, &alphabet, Delta(1));
//
//         A.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 0, {});
//
//         Nfta C = union_product(A, B);
//
//         CHECK(C.delta.num_of_states() == 1);
//         CHECK(C.is_state_final(0));
//     }
//
//     SECTION("Two states each, reachable product states only") {
//         Nfta A({}, &alphabet, Delta(2));
//         Nfta B({}, &alphabet, Delta(2));
//
//         // A transitions
//         A.delta.add(alphabet["f"], 0, {1});
//         A.delta.add(alphabet["f"], 1, {1});
//         A.delta.add(alphabet["a"], 0, {});
//         A.delta.add(alphabet["a"], 1, {});
//
//         // B transitions
//         B.delta.add(alphabet["f"], 0, {0});
//         B.delta.add(alphabet["f"], 1, {1});
//         B.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 1, {});
//
//         A.add_final_state(1);
//         B.add_final_state(0);
//
//         Nfta C = union_product(A, B);
//
//         // At most 4 product states, but maybe fewer if unreachable
//         CHECK(C.delta.num_of_states() <= 4);
//
//         // At least one final (union condition)
//         CHECK_FALSE(C.get_final_states().empty());
//     }
//
//     SECTION("Union condition correctness") {
//         Nfta A({}, &alphabet, Delta(2));
//         Nfta B({}, &alphabet, Delta(2));
//
//         A.delta.add(alphabet["a"], 0, {});
//         A.delta.add(alphabet["a"], 1, {});
//         B.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 1, {});
//
//         A.add_final_state(1); // only state 1 final
//         B.add_final_state(0); // only state 0 final
//
//         Nfta C = union_product(A, B);
//
//         // Any product state involving A=1 OR B=0 must be final
//         bool found_expected_final = false;
//
//         for (State s = 0; s < C.delta.num_of_states(); ++s) {
//             if (C.is_state_final(s)) {
//                 found_expected_final = true;
//                 break;
//             }
//         }
//
//         CHECK(found_expected_final);
//     }
//
//     SECTION("Branching targets (arity > 1)") {
//         Nfta A({}, &alphabet, Delta(2));
//         Nfta B({}, &alphabet, Delta(2));
//
//         A.delta.add(alphabet["f"], 0, {0,1});
//         A.delta.add(alphabet["f"], 1, {1,1});
//         A.delta.add(alphabet["a"], 0, {});
//         A.delta.add(alphabet["a"], 1, {});
//
//         B.delta.add(alphabet["f"], 0, {1,0});
//         B.delta.add(alphabet["f"], 1, {0,0});
//         B.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 1, {});
//
//         B.add_final_state(1);
//
//         Nfta C = union_product(A, B);
//
//         CHECK(C.delta.num_of_states() > 0);
//         CHECK_FALSE(C.get_final_states().empty());
//     }
//
//     SECTION("No unreachable product states created") {
//         Nfta A({}, &alphabet, Delta(2));
//         Nfta B({}, &alphabet, Delta(2));
//
//         // Only state 0 reachable via 'a'
//         A.delta.add(alphabet["a"], 0, {});
//         A.delta.add(alphabet["a"], 1, {});
//         B.delta.add(alphabet["a"], 0, {});
//         B.delta.add(alphabet["a"], 1, {});
//
//         Nfta C = union_product(A, B);
//
//         // Should only create state (0,0)
//         CHECK(C.delta.num_of_states() == 1);
//     }
// }
