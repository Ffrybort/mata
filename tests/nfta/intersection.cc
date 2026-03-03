#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata;

TEST_CASE("mata::nfta::intersection_product") {

    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("a");

    SECTION("No final states") {
        Nfta A({}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        Nfta C = intersection(A, B);

        CHECK(C.is_empty());
    }

    SECTION("Single final state only on one side") {
        Nfta A({0}, &alphabet, Delta(1));
        Nfta B({}, &alphabet, Delta(1));

        // A accepts something, B does not
        A.delta.add(0, alphabet["a"], {});

        Nfta C = intersection(A, B);

        CHECK(C.is_empty());
    }

    SECTION("Identical automata") {
        Nfta A({0}, &alphabet, Delta(1));
        Nfta B({0}, &alphabet, Delta(1));

        A.delta.add(0, alphabet["a"], {});
        B.delta.add(0, alphabet["a"], {});

        Nfta C = intersection(A, B);

        CHECK(C.delta.num_of_states() == 1);
        CHECK(C.is_state_final(0));
        CHECK(C.delta.contains(0, alphabet["a"], {}));
    }

    SECTION("Intersection – 3-state reachable product") {

        Nfta A({1}, &alphabet, Delta(2));
        Nfta B({0}, &alphabet, Delta(2));

         // A: final = {1}
         // 0 --f--> 1
         // 1 --f--> 0
         //
         // B: final = {0}
         // 0 --f--> 1
         // 1 --f--> 1

        A.delta.add(0, alphabet["f"], {1});
        A.delta.add(1, alphabet["f"], {0});
        A.delta.add(1, alphabet["a"], {});
        A.add_final_state(1);

        B.delta.add(0, alphabet["f"], {1});
        B.delta.add(1, alphabet["f"], {1});
        B.delta.add(1, alphabet["a"], {});
        B.add_final_state(0);

        Nfta C = intersection(A, B);

        // C states
        // (1, 0) 0 final
        // (0, 1) 1
        // (1, 1) 2
        // (0, 0) is unreachable
        REQUIRE(C.delta.num_of_states() == 3);
        CHECK(C.is_state_final(0));

        // Transition checks
        CHECK(C.delta.contains(0, alphabet["f"], {1}));
        CHECK(C.delta.contains(1, alphabet["f"], {2}));
        CHECK(C.delta.contains(2, alphabet["f"], {1}));
        CHECK(C.delta.contains(2, alphabet["a"], {}));
    }
}
