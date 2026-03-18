#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

TEST_CASE("mata::nfta::determinize_naive") {
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("f"); // unary
    alphabet.add_new_symbol("a"); // constant
    alphabet.add_new_symbol("g"); // binary
    alphabet.add_new_symbol("h"); // 3
    alphabet.add_new_symbol("k"); // 4

    SECTION("Empty") {
        Nfta aut({}, &alphabet, {});

        aut.determinize(nullptr);

        CHECK(aut.is_bottom_up_deterministic());
        CHECK(aut.delta.empty());
    }

    SECTION("Already deterministic") {
        Nfta aut({0}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["f"], {0});

        size_t before_states = aut.delta.num_of_states();

        aut.determinize(nullptr);

        CHECK(aut.is_bottom_up_deterministic());
        CHECK(aut.delta.num_of_transitions() == 2);
        CHECK(aut.delta.num_of_states() == before_states);
    }

    SECTION("Constant nondeterminism") {
        Nfta aut({}, &alphabet, Delta(2));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        // expect macrostate {0,1}
        bool found = false;

        for (const auto& [set, det_state] : mapping) {
            if (set.size() == 2 &&
                set.at(0) == 0 &&
                set.at(1) == 1) {
                found = true;
            }
        }

        CHECK(found);
    }

    SECTION("Unary nondeterminism") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});

        aut.delta.add(1, alphabet["f"], {0});
        aut.delta.add(2, alphabet["f"], {0});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        // we should have at least two macrostates
        CHECK(mapping.size() >= 2);

        for (const auto& [set, det_state] : mapping) {
            CHECK(set.size() >= 1);
        }
    }

    SECTION("Two-level nondeterminism") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(2, alphabet["f"], {1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        bool found01 = false;

        for (const auto& [set, det_state] : mapping) {
            if (set.size() == 2 &&
                set.at(0) == 0 &&
                set.at(1) == 1) {
                found01 = true;
            }
        }

        CHECK(found01);
    }

    SECTION("Chain nondeterminism") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        // ensure mapping entries are valid
        for (const auto& [set, det_state] : mapping) {
            CHECK(set.size() >= 1);
            CHECK(det_state < aut.delta.num_of_states());
        }
    }

    SECTION("Unary chain") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {1});
        aut.delta.add(3, alphabet["f"], {0});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        // ensure each macrostate maps to valid deterministic state
        for (const auto& det_state : mapping | std::views::values) {
            CHECK(det_state < aut.delta.num_of_states());
        }
    }

    SECTION("Larger unary - check exact macrostates") {
        Nfta aut({4}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {0});
        aut.delta.add(4, alphabet["f"], {1});
        aut.delta.add(4, alphabet["f"], {2});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());
        CHECK_FALSE(mapping.empty());

        std::vector<OrdVector<State>> macrosets;
        for (const auto& set : mapping | std::views::keys) {
            macrosets.push_back(set);
        }

        // Expected macrostate sets (we know from the transitions)
        std::vector<OrdVector<State>> expected {
            OrdVector<State>{0, 1},
            OrdVector<State>{2, 3, 4},
            {4}
        };

        for (const auto& exp : expected) {
            bool found = false;
            for (const auto& m : macrosets) { if (m == exp) { found = true; break; } }
            CHECK(found);
        }

        for (const auto& det_state : mapping | std::views::values) {
            CHECK(det_state < aut.delta.num_of_states());
        }
    }

    SECTION("Binary deterministic") {
        Nfta aut({}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(2, alphabet["g"], {0,0});

        aut.determinize(nullptr);

        CHECK(aut.is_bottom_up_deterministic());
        CHECK(aut.delta.num_of_transitions() == 2);
    }

    SECTION("Same tuple") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["g"], {0,1});
        aut.delta.add(3, alphabet["g"], {0,1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        bool found = false;

        for (const auto& set : mapping | std::views::keys) {
            if (set.size() == 2 &&
                set.at(0) == 2 &&
                set.at(1) == 3) {
                found = true;
                }
        }
        CHECK(found);
    }

    SECTION("Nondeterminism in constants") {
        Nfta aut({}, &alphabet, Delta(4));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["g"], {0,0});
        aut.delta.add(3, alphabet["g"], {1,1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        bool found = false;

        for (const auto& set : mapping | std::views::keys) {
            if (set.size() == 2 &&
                set.contains(2) &&
                set.contains(3)) {
                found = true;
                }
        }

        CHECK(found);
    }

    SECTION("Binary tuple expansion") {
        Nfta aut({}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["g"], {0,1});
        aut.delta.add(3, alphabet["g"], {1,0});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        bool found = false;

        for (const auto& set : mapping | std::views::keys) {
            if (set.size() == 2 &&
                set.contains(2) &&
                set.contains(3)) {
                found = true;
                }
        }

        CHECK(found);
    }

    SECTION("Binary and unary transitions") {
        Nfta aut({}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {1});

        aut.delta.add(4, alphabet["g"], {2,3});
        aut.delta.add(4, alphabet["g"], {3,2});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() == 3);
        CHECK(aut.delta.num_of_states() == 3);
        CHECK(aut.delta.num_of_transitions() == 3);

        bool found01=false, found23=false, found4=false;

        for (const auto& set : mapping | std::views::keys) {
            if (set == OrdVector<State>{0,1}) found01=true;
            if (set == OrdVector<State>{2,3}) found23=true;
            if (set == OrdVector<State>{4}) found4=true;
        }

        CHECK(found01);
        CHECK(found23);
        CHECK(found4);

        for (const auto& [set, det] : mapping) {
            CHECK(det < aut.delta.num_of_states());
            CHECK_FALSE(set.empty());
        }
    }

    SECTION("Repeated state tuple") {
        Nfta aut({1}, &alphabet, Delta(3));

        aut.delta.add(0, alphabet["a"], {});

        aut.delta.add(1, alphabet["g"], {0,0});
        aut.delta.add(2, alphabet["g"], {0,0});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() == 2);
        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.num_of_transitions() == 2);
        CHECK(aut.is_state_initial(mapping[{1, 2}]));

        bool found = false;

        for (const auto& set : mapping | std::views::keys) {
            if (set == OrdVector<State>{1,2})
                found = true;
        }
        CHECK(found);
    }

    SECTION("Ternary permutations") {
        Nfta aut({4}, &alphabet, Delta(6));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["h"], {0,0,1});
        aut.delta.add(3, alphabet["h"], {1,0,0});
        aut.delta.add(4, alphabet["h"], {0,1,0});
        aut.delta.add(5, alphabet["h"], {1,1,1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() == 2);
        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.num_of_transitions() == 2);

        bool found = false;

        for (const auto& set : mapping | std::views::keys) {
            if (set == OrdVector<State>{2,3,4,5})
                found=true;
        }

        CHECK(found);
    }

    SECTION("Ternary repeated states") {
        Nfta aut({4}, &alphabet, Delta(5));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["h"], {0,0,0});
        aut.delta.add(3, alphabet["h"], {0,0,0});
        aut.delta.add(4, alphabet["h"], {1,1,1});

        std::unordered_map<StateSet, State> mapping;
        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() == 2);
        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.num_of_transitions() == 2);
        CHECK(aut.is_state_initial(mapping[{2, 3, 4}]));
    }

    SECTION("Ternary chain") {
        Nfta aut({7, 2}, &alphabet, Delta(8));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {1});

        aut.delta.add(4, alphabet["h"], {2,3,2});
        aut.delta.add(5, alphabet["h"], {3,2,3});
        aut.delta.add(6, alphabet["h"], {2,2,3});
        aut.delta.add(7, alphabet["h"], {3,3,2});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() >= 3);
        CHECK(aut.delta.num_of_states() >= 3);
        CHECK(aut.delta.num_of_transitions() >= 3);
        CHECK(aut.is_state_initial(mapping[{2, 3}]));
        CHECK(aut.is_state_initial(mapping[{4, 5, 6, 7}]));
    }

    SECTION("Arity 4 combinatorial") {
        Nfta aut({10}, &alphabet, Delta(11));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["k"], {0,0,0,0});
        aut.delta.add(3, alphabet["k"], {1,1,1,1});
        aut.delta.add(4, alphabet["k"], {0,1,0,1});
        aut.delta.add(5, alphabet["k"], {1,0,1,0});
        aut.delta.add(6, alphabet["k"], {0,0,1,1});
        aut.delta.add(7, alphabet["k"], {1,1,0,0});
        aut.delta.add(8, alphabet["k"], {0,1,1,0});
        aut.delta.add(9, alphabet["k"], {1,0,0,1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() >= 2);
        CHECK(aut.delta.num_of_states() == 2);
        CHECK(aut.delta.num_of_transitions() == 2);
        CHECK(aut.initial_states.empty());
    }

    SECTION("Propagation") {
        Nfta aut({}, &alphabet, Delta(9));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});

        aut.delta.add(2, alphabet["f"], {0});
        aut.delta.add(3, alphabet["f"], {1});

        aut.delta.add(4, alphabet["h"], {2,2,3});
        aut.delta.add(5, alphabet["h"], {2,3,3});

        aut.delta.add(6, alphabet["k"], {4,5,4,5});
        aut.delta.add(7, alphabet["k"], {5,4,5,4});
        aut.delta.add(8, alphabet["k"], {4,4,5,5});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());
        CHECK(aut.delta.num_of_transitions() == 4);
    }

    SECTION("Simple") {
        Nfta aut({}, &alphabet, Delta(7));

        aut.delta.add(0, alphabet["a"], {});
        aut.delta.add(1, alphabet["a"], {});
        aut.delta.add(2, alphabet["a"], {});

        aut.delta.add(3, alphabet["h"], {0,1,2});
        aut.delta.add(4, alphabet["h"], {1,2,0});
        aut.delta.add(5, alphabet["h"], {2,0,1});
        aut.delta.add(6, alphabet["h"], {0,2,1});

        std::unordered_map<StateSet, State> mapping;

        aut.determinize(&mapping);

        CHECK(aut.is_bottom_up_deterministic());

        CHECK(mapping.size() == 2);
        CHECK(aut.delta.num_of_states() == 2);

        CHECK(aut.delta.num_of_transitions() == 2);
        CHECK(aut.delta.contains(0, alphabet["a"], {}));
        CHECK(aut.delta.contains(1, alphabet["h"], {0,0,0}));
    }
}