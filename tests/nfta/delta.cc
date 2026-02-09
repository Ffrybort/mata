/**
* Basic delta functionality
*/
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;
TEST_CASE("Delta basic functionality") {

    Delta delta;

    SECTION("Add and Contains transitions") {
        delta.add(0, 1, {2, 3});
        delta.add(0, 2, {4});

        CHECK(delta.contains(0, 1, {2, 3}));
        CHECK(delta.contains(0, 2, {4}));
        CHECK_FALSE(delta.contains(0, 1, {3, 2})); // order matters
        CHECK_FALSE(delta.contains(1, 1, {2, 3})); // wrong source
    }

    SECTION("Remove transitions") {
        delta.add(0, 1, {2});
        CHECK(delta.contains(0, 1, {2}));

        delta.remove(0, 1, {2});
        CHECK_FALSE(delta.contains(0, 1, {2}));

        // Removing non-existent transition throws
        CHECK_THROWS_AS(delta.remove(0, 1, {2}), std::invalid_argument);
    }

    SECTION("Number of transitions") {
        delta.add_multiple(0, 1, {{2},{3}});
        delta.add(1, 2, {3});
        delta.add(1, 2, {4}); // same symbol, added separately

        CHECK(delta.num_of_transitions() == 4);
    }

    SECTION("Get all transitions") {
        delta.add(0, 1, {2});
        delta.add(0, 2, {3});
        delta.add(1, 1, {0});

        auto transitions = delta.get_transitions();
        CHECK(transitions.size() == 3);

        CHECK(transitions[0].source == 0);
        CHECK(transitions[0].symbol == 1);
        CHECK(transitions[0].targets == std::vector<State>{2});

        CHECK(transitions[1].source == 0);
        CHECK(transitions[1].symbol == 2);
        CHECK(transitions[1].targets == std::vector<State>{3});

        CHECK(transitions[2].source == 1);
        CHECK(transitions[2].symbol == 1);
        CHECK(transitions[2].targets == std::vector<State>{0});
    }

    SECTION("Get transitions to a specific state vector") {
        delta.add(0, 1, {2,3});
        delta.add(1, 2, {3});
        delta.add(2, 1, {2,3});

        auto transitions_to_2_3 = delta.get_transitions_to({2,3});
        CHECK(transitions_to_2_3.size() == 2);

        std::vector<State> targets0 = transitions_to_2_3[0].targets;
        std::vector<State> targets1 = transitions_to_2_3[1].targets;

        CHECK((targets0 == std::vector<State>{2,3} || targets1 == std::vector<State>{2,3}));
    }

    SECTION("Get successors") {
        delta.add(0, 1, {2});
        delta.add(0, 1, {2});
        delta.add(0, 2, {3});
        delta.add(1, 1, {0});

        auto successors0 = delta.get_successors(0);
        CHECK(successors0.count({2}) == 1);
        CHECK(successors0.count({3}) == 1);

        auto successors0_sym1 = delta.get_successors(0, 1);
        CHECK(successors0_sym1.count({2}) == 1);
        CHECK(successors0_sym1.count({3}) == 0);

        auto successors1 = delta.get_successors(1);
        CHECK(successors1.count({0}) == 1);
    }

    SECTION("Moves iteration over a single StatePost") {
        delta.add(0, 1, {2,3});
        delta.add(0, 2, {0, 1, 2, 3, 4});
        delta.add(0, 3, {4});

        const StatePost& sp = delta.state_post(0);
        auto moves = sp.moves();

        std::vector<std::pair<Symbol, std::vector<State>>> collected;
        for (auto it = moves.begin(); it != moves.end(); ++it) {
            collected.emplace_back(it->symbol, it->targets);
        }

        CHECK(collected.size() == 3);
        CHECK(collected[0].first == 1);
        CHECK(collected[0].second == std::vector<State>{2, 3});
        CHECK(collected[1].first == 2);
        CHECK(collected[1].second == std::vector<State>{0, 1, 2, 3, 4});
        CHECK(collected[2].first == 3);
        CHECK(collected[2].second == std::vector<State>{4});
    }

    SECTION("Empty delta") {
        CHECK(delta.is_empty());

        delta.add(0, 1, {2});
        CHECK_FALSE(delta.is_empty());
    }
}
