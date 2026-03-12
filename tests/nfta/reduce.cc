#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;


TEST_CASE("mata::nfta::accessibility") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // constant
    alphabet.add_new_symbol("f"); // unary
    alphabet.add_new_symbol("g"); // binary

    SECTION("Top-down simple chain") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc.size() == 3);
        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
    }

    SECTION("Top-down unreachable state") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(2, alphabet["f"], {1});
        // state 2 unreachable

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Top-down branching") {
        Nfta aut({0}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["g"], {1,2});
        aut.delta.add(2, alphabet["f"], {3});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
        CHECK(acc[3]);
    }

    SECTION("No initial states") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK_FALSE(acc[0]);
        CHECK_FALSE(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Self loop") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["f"], {0});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK_FALSE(acc[1]);
    }

    SECTION("Cycle between states") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(2, alphabet["f"], {0});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
    }

    SECTION("Multiple initial states") {
        Nfta aut({0,2}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(2, alphabet["f"], {3});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
        CHECK(acc[3]);
    }

    SECTION("Unreachable island") {
        Nfta aut({0}, &alphabet, Delta(6));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});

        aut.delta.add(3, alphabet["f"], {4});
        aut.delta.add(3, alphabet["g"], {4, 5});
        aut.delta.add(4, alphabet["g"], {4, 5});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
        CHECK_FALSE(acc[3]);
        CHECK_FALSE(acc[4]);
        CHECK_FALSE(acc[5]);
    }

    SECTION("Constant transitions do not create successors") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {}); // constant
        aut.delta.add(1, alphabet["f"], {2}); // unreachable

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK_FALSE(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Repeated successors") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["g"], {1,1});
        aut.delta.add(1, alphabet["f"], {2});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
    }


    // SECTION("Bottom-up constant start") {
    //     Nfta aut({}, &alphabet, Delta(3));
    //
    //     aut.delta.add(0, alphabet["a"], {}); // constant
    //     aut.delta.add(1, alphabet["f"], {0});
    //     aut.delta.add(2, alphabet["f"], {1});
    //
    //     BoolVector acc = aut.get_bottom_up_reachable();
    //
    //     CHECK(acc[0]);
    //     CHECK(acc[1]);
    //     CHECK(acc[2]);
    // }
    //
    // SECTION("Bottom-up unreachable") {
    //     Nfta aut({}, &alphabet, Delta(3));
    //
    //     aut.delta.add(0, alphabet["a"], {});
    //     aut.delta.add(1, alphabet["f"], {0});
    //     // state 2 never constructed
    //
    //     BoolVector acc = aut.get_bottom_up_reachable();
    //
    //     CHECK(acc[0]);
    //     CHECK(acc[1]);
    //     CHECK_FALSE(acc[2]);
    // }
    //
    // SECTION("Bottom-up binary propagation") {
    //     Nfta aut({}, &alphabet, Delta(4));
    //
    //     aut.delta.add(0, alphabet["a"], {});
    //     aut.delta.add(1, alphabet["a"], {});
    //
    //     aut.delta.add(2, alphabet["g"], {0,1});
    //     aut.delta.add(3, alphabet["f"], {2});
    //
    //     BoolVector acc = aut.get_bottom_up_reachable();
    //
    //     CHECK(acc[0]);
    //     CHECK(acc[1]);
    //     CHECK(acc[2]);
    //     CHECK(acc[3]);
    // }

    // SECTION("Bottom-up missing child blocks accessibility") {
    //     Nfta aut({}, &alphabet, Delta(3));
    //
    //     aut.delta.add(0, alphabet["a"], {});
    //     aut.delta.add(2, alphabet["g"], {0,1}); // 1 missing
    //
    //     BoolVector acc = aut.get_bottom_up_reachable();
    //
    //     CHECK(acc[0]);
    //     CHECK_FALSE(acc[1]);
    //     CHECK_FALSE(acc[2]);
    // }
    //
    // SECTION("Bottom-up multiple constants") {
    //     Nfta aut({}, &alphabet, Delta(5));
    //
    //     aut.delta.add(0, alphabet["a"], {});
    //     aut.delta.add(1, alphabet["a"], {});
    //
    //     aut.delta.add(2, alphabet["g"], {0,1});
    //     aut.delta.add(3, alphabet["g"], {1,0});
    //     aut.delta.add(4, alphabet["f"], {3});
    //
    //     BoolVector acc = aut.get_bottom_up_reachable();
    //
    //     CHECK(acc[0]);
    //     CHECK(acc[1]);
    //     CHECK(acc[2]);
    //     CHECK(acc[3]);
    //     CHECK(acc[4]);
    // }
}