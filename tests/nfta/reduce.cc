#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;


TEST_CASE("mata::nfta::get_top_down_reachable") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // constant
    alphabet.add_new_symbol("f"); // unary
    alphabet.add_new_symbol("g"); // binary

    SECTION("Empty") {
        Nfta aut({0}, &alphabet, Delta(3));

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK_FALSE(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Simple chain") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc.size() == 3);
        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
    }

    SECTION("Unreachable state") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(2, alphabet["f"], {1});
        // state 2 unreachable

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Branching") {
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

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {2});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK_FALSE(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Another simple") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["g"], {1,1});
        aut.delta.add(1, alphabet["f"], {2});

        BoolVector acc = aut.get_top_down_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
    }
}
TEST_CASE("mata::nfta::get_bottom_up_reachable") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // constant
    alphabet.add_new_symbol("f"); // unary
    alphabet.add_new_symbol("g"); // binary

    SECTION("Empty") {
        Nfta aut({0}, &alphabet, Delta(1));
        aut.delta.add(0, alphabet["a"], {});

        BoolVector acc = aut.get_bottom_up_reachable();

        CHECK(acc[0]);
    }

    SECTION("Constant only") {
        Nfta aut({}, &alphabet, Delta(3));
        BoolVector acc = aut.get_bottom_up_reachable();

        CHECK_FALSE(acc[0]);
        CHECK_FALSE(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Bottom-up simple") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(2, alphabet["f"], {1});

        BoolVector acc = aut.get_bottom_up_reachable();
        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
    }

    SECTION("Bottom-up unreachable") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});
        // state 2 never constructed

        BoolVector acc = aut.get_bottom_up_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Bottom-up binary") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["g"], {0,1});
        aut.delta.add(3, alphabet["f"], {2});

        BoolVector acc = aut.get_bottom_up_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
        CHECK(acc[3]);
    }

    SECTION("Bottom-up missing child") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(2, alphabet["g"], {0,1}); // 1 missing

        BoolVector acc = aut.get_bottom_up_reachable();

        CHECK(acc[0]);
        CHECK_FALSE(acc[1]);
        CHECK_FALSE(acc[2]);
    }

    SECTION("Bottom-up multiple constant transitions") {
        Nfta aut({}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["g"], {0,1});
        aut.delta.add(3, alphabet["g"], {1,0});
        aut.delta.add(4, alphabet["f"], {3});

        BoolVector acc = aut.get_bottom_up_reachable();

        CHECK(acc[0]);
        CHECK(acc[1]);
        CHECK(acc[2]);
        CHECK(acc[3]);
        CHECK(acc[4]);
    }

    alphabet.add_new_symbol("b"); // constant
    alphabet.add_new_symbol("h"); // ternary
    alphabet.add_new_symbol("k"); // quaternary

    SECTION("Simple constants") {
        Nfta aut({}, &alphabet, Delta(3));

        // Only one constant reachable
        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["b"], {});
        aut.delta.add(2, alphabet["f"], {0});

        BoolVector reachable = aut.get_bottom_up_reachable();
        CHECK(reachable[0]);
        CHECK(reachable[1]);
        CHECK(reachable[2]);
    }

    SECTION("Multiple levels, some unreachable") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(2, alphabet["g"], {0,1});
        aut.delta.add(3, alphabet["h"], {1,2,3});

        BoolVector reachable = aut.get_bottom_up_reachable();
        CHECK(reachable[0]);
        CHECK(reachable[1]);
        CHECK(reachable[2]);
        CHECK_FALSE(reachable[3]);
    }

    SECTION("Disconnected components") {
        Nfta aut({}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});

        aut.delta.add(2, alphabet["b"], {});
        aut.delta.add(3, alphabet["f"], {2});

        BoolVector reachable = aut.get_bottom_up_reachable();
        CHECK(reachable[0]);
        CHECK(reachable[1]);
        CHECK(reachable[2]);
        CHECK(reachable[3]);
        CHECK_FALSE(reachable[4]);
    }

    SECTION("Multiple initial states") {
        Nfta aut({}, &alphabet, Delta(6));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["b"], {});
        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {1});
        aut.delta.add(4, alphabet["g"], {4,4});
        aut.delta.add(5, alphabet["h"], {0,1,2});

        BoolVector reachable = aut.get_bottom_up_reachable();
        CHECK(reachable[0]);
        CHECK(reachable[1]);
        CHECK(reachable[2]);
        CHECK(reachable[3]);
        CHECK_FALSE(reachable[4]);
        CHECK(reachable[5]);
    }

    SECTION("Larger automaton") {
        constexpr size_t N = 10;
        Nfta aut({}, &alphabet, Delta(N));
        for (State s = 0; s < 3; ++s) {aut.delta.add(s, alphabet["a"], {}); }
        for (State s = 3; s < 6; ++s) {aut.delta.add(s, alphabet["f"], {s-3}); }

        aut.delta.add(6, alphabet["g"], {0,1});
        aut.delta.add(7, alphabet["g"], {1,2});
        aut.delta.add(8, alphabet["g"], {3,4});
        aut.delta.add(9, alphabet["h"], {5,6,7});

        BoolVector reachable = aut.get_bottom_up_reachable();

        for (State s = 0; s < N; ++s) {
            CHECK(reachable[s]);
        }
    }
}

TEST_CASE("mata::nfta::reduce_top_down") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // arity 0
    alphabet.add_new_symbol("f"); // arity 1

    SECTION("Empty") {
        Nfta aut({}, &alphabet, Delta(10));

        aut.reduce_top_down();
        CHECK(aut.delta.num_of_states() == 0);
    }

    SECTION("Final state only") {
        Nfta aut({0}, &alphabet, Delta(42));

        aut.reduce_top_down();
        CHECK(aut.delta.num_of_states() == 1);
    }


    SECTION("Simple reduction") {
        Nfta aut({0}, &alphabet, Delta(3));

        aut.delta.add(1, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(2, alphabet["a"], {});

        aut.reduce_top_down();
        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.contains(0, alphabet["f"], {1}));
        CHECK(aut.delta.contains(1, alphabet["a"], {}));
    }

    SECTION("Reachable chain") {
        Nfta aut({0}, &alphabet, Delta(4));

        aut.delta.add(3, alphabet["a"], {});
        aut.delta.add(2, alphabet["f"], {3});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(3, alphabet["f"], {3});

        aut.reduce_top_down();
        CHECK(aut.delta.num_of_states() == 4);
    }

    SECTION("Multiple initial") {
        Nfta aut({0, 1}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(3, alphabet["a"], {});

        aut.reduce_top_down();

        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.contains(0, alphabet["a"], {}));
        CHECK(aut.delta.contains(1, alphabet["a"], {}));
    }
}

TEST_CASE("mata::nfta::reduce_bottom_up") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // 0
    alphabet.add_new_symbol("f"); // 1
    alphabet.add_new_symbol("g"); // 2

    SECTION("Empty") {
        Nfta aut({}, &alphabet, Delta(10));

        aut.reduce_bottom_up();
        CHECK(aut.delta.num_of_states() == 0);
    }

    SECTION("Simple reduction removes unreachable states") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(1, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(3, alphabet["f"], {2});

        aut.reduce_bottom_up();

        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.contains(1, alphabet["a"], {}));
        CHECK(aut.delta.contains(0, alphabet["f"], {1}));
    }

    SECTION("Reachable chain") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(0, alphabet["f"], {1});

        aut.reduce_bottom_up();

        CHECK(aut.delta.num_of_states() == 3);
    }

    SECTION("No constant") {
        Nfta aut({}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {0});

        aut.reduce_bottom_up();

        CHECK(aut.delta.num_of_states() == 0);
    }

    SECTION("Partially unreachable states") {
        Nfta aut({0}, &alphabet, Delta(5));

        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(0, alphabet["g"], {1, 2});
        aut.delta.add(0, alphabet["g"], {3, 2});

        aut.reduce_bottom_up();

        CHECK(aut.delta.num_of_states() == 3);
    }

    SECTION("Unreachable component") {
        Nfta aut({}, &alphabet, Delta(10));

        aut.delta.add(3, alphabet["a"], {});
        aut.delta.add(2, alphabet["f"], {3});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(0, alphabet["f"], {1});

        aut.delta.add(7, alphabet["f"], {6});
        aut.delta.add(6, alphabet["f"], {7});
        aut.delta.add(5, alphabet["f"], {6});
        aut.delta.add(4, alphabet["f"], {5});

        aut.reduce_bottom_up();

        CHECK(aut.delta.num_of_states() == 4);
    }
}

TEST_CASE("mata::nfta::reduce_top_bottom_top and reduce_bottom_top") { // todo check with equality
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a"); // arity 0
    alphabet.add_new_symbol("f"); // arity 1
    alphabet.add_new_symbol("g"); // arity 2

    // helper — run both reductions on separate copies and check they agree
    auto check_both = [](Nfta aut_tbt, Nfta aut_bt, auto check_fn) {
        aut_tbt.reduce_top_bottom_top();
        aut_bt.reduce_bottom_top();
        check_fn(aut_tbt);
        check_fn(aut_bt);
    };

    SECTION("Empty automaton") {
        Nfta aut({}, &alphabet, Delta(10));
        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 0);
        });
    }

    SECTION("Only initial state, no transitions") {
        Nfta aut({0}, &alphabet, Delta(5));
        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 0);
        });
    }

    SECTION("Simple — all states useful") {
        Nfta aut({0}, &alphabet, Delta(3));
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(2, alphabet["a"], {});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 3);
        });
    }

    SECTION("State reachable top-down but not bottom-up — removed") {
        Nfta aut({0}, &alphabet, Delta(4));
        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});
        // state 2 has no leaf, so 1 and 2 are dead

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 1);
        });
    }

    SECTION("State bottom-up reachable but not top-down — removed") {
        Nfta aut({0}, &alphabet, Delta(4));
        aut.delta.add(0, alphabet["a"], {});
        // states 1,2,3 are bottom-up reachable but unreachable from initial state 0
        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(3, alphabet["f"], {1});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 1);
        });
    }

    SECTION("Binary — both children must be reachable") {
        Nfta aut({0}, &alphabet, Delta(5));
        aut.delta.add(1, alphabet["a"], {});
        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(0, alphabet["g"], {1, 2});
        aut.delta.add(3, alphabet["a"], {});
        aut.delta.add(4, alphabet["f"], {3});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 3);
        });
    }

    SECTION("Dead branch") {
        Nfta aut({0}, &alphabet, Delta(6));
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(2, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {3});
        aut.delta.add(3, alphabet["f"], {4});
        aut.delta.add(5, alphabet["a"], {});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 3);
        });
    }

    SECTION("Cycle — all reachable") {
        Nfta aut({0}, &alphabet, Delta(3));
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(1, alphabet["a"], {});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 2);
        });
    }

    SECTION("Cycle without leaf — all removed") {
        Nfta aut({0}, &alphabet, Delta(4));
        aut.delta.add(0, alphabet["f"], {1});
        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(3, alphabet["a"], {});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 0);
        });
    }

    SECTION("Multiple initial states — partial reduction") {
        Nfta aut({0, 1}, &alphabet, Delta(5));
        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {2});
        aut.delta.add(2, alphabet["a"], {});
        // states 3, 4 unreachable
        aut.delta.add(3, alphabet["a"], {});
        aut.delta.add(4, alphabet["f"], {3});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 3);
        });
    }

    SECTION("Mixed reachability") {
        Nfta aut({0}, &alphabet, Delta(8));
        aut.delta.add(0, alphabet["g"], {1, 2});
        aut.delta.add(1, alphabet["a"], {});
        aut.delta.add(2, alphabet["f"], {3});
        aut.delta.add(3, alphabet["a"], {});
        aut.delta.add(0, alphabet["f"], {4});
        aut.delta.add(4, alphabet["f"], {5});
        aut.delta.add(6, alphabet["a"], {});
        aut.delta.add(7, alphabet["f"], {6});

        check_both(aut, aut, [](const Nfta& a) {
            CHECK(a.delta.num_of_states() == 4);
        });
    }
}
