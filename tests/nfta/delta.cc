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
TEST_CASE("mata::nfta::delta") {
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
        delta.add(0, SymbolPost{ 1, StateVectorSet{{2},{3}} });
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

        CHECK(std::any_of(transitions.begin(), transitions.end(),
            [](const Transition& t){
                return t.source == 0 && t.symbol == 1 && t.targets == std::vector<State>{2};
            }));

        CHECK(std::any_of(transitions.begin(), transitions.end(),
            [](const Transition& t){
                return t.source == 0 && t.symbol == 2 && t.targets == std::vector<State>{3};
            }));

        CHECK(std::any_of(transitions.begin(), transitions.end(),
            [](const Transition& t){
                return t.source == 1 && t.symbol == 1 && t.targets == std::vector<State>{0};
            }));

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


    SECTION("Equality and inequality") {
        Move m1{1, {2,3}};
        Move m2{1, {2,3}};
        Move m3{1, {3,2}};
        Move m4{2, {2,3}};

        CHECK(m1 == m2);
        CHECK_FALSE(m1 == m3);
        CHECK_FALSE(m1 == m4);
    }

    delta.clear();

    SECTION("Empty StatePost produces no moves") {
        const StatePost& sp = delta.state_post(0);
        auto moves = sp.moves();

        CHECK(moves.begin() == StatePost::Moves::end());
    }

    SECTION("Multiple target tuples under same symbol") {
        delta.add(0, 1, {2});
        delta.add(0, 1, {3});

        const StatePost& sp = delta.state_post(0);

        size_t count = 0;
        for (auto it = sp.moves().begin(); it != StatePost::Moves::end(); it++) {
            ++count;
        }

        CHECK(count == 2);
        CHECK(sp.num_of_moves() == 2);
    }

    SECTION("Postfix increment works") {
        delta.add(0, 1, {2});
        delta.add(0, 2, {3});

        const StatePost& sp = delta.state_post(0);
        auto it = sp.moves().begin();

        auto first = *it++;
        CHECK(first.symbol == 1);

        auto second = *it;
        CHECK(second.symbol == 2);
    }

    SECTION("Iterator equality semantics") {
        delta.add(0, 1, {2});

        const StatePost& sp = delta.state_post(0);
        auto it1 = sp.moves().begin();
        auto it2 = sp.moves().begin();

        CHECK(it1 == it2);

        ++it1;
        CHECK(it1 == StatePost::Moves::end());
    }

    delta.clear();
    delta.add(0, 1, {2});
    delta.add(1, 2, {3});

    auto transitions = delta.transitions();

    std::vector<Transition> collected;

    for (auto it = transitions.begin(); it != Delta::Transitions::end(); ++it) {
        collected.push_back(*it);
    }

    CHECK(collected.size() == 2);

    CHECK(collected[0].source == 0);
    CHECK(collected[0].symbol == 1);
    CHECK(collected[0].targets == std::vector<State>{2});

    CHECK(collected[1].source == 1);
    CHECK(collected[1].symbol == 2);
    CHECK(collected[1].targets == std::vector<State>{3});

    delta.clear();

    SECTION("add_state explicit index") {
        delta.add_state(5);
        CHECK(delta.num_of_states() == 6);
        CHECK(delta.contains_state(5));
    }

    SECTION("add_state auto increment") {
        State s = delta.add_state();
        CHECK(s == 0);
        CHECK(delta.num_of_states() == 1);
    }

    SECTION("resize_for_states variadic") {
        delta.resize_for_states(3u, 7u);
        CHECK(delta.num_of_states() == 8);
    }

    SECTION("contains_state") {
        delta.add_state(2);
        CHECK(delta.contains_state(2));
        CHECK_FALSE(delta.contains_state(5));
    }

    Delta d1;
    d1.add(0, 1, {2});

    SECTION("Copy constructor") {
        Delta d2{d1};
        CHECK(d2 == d1);
    }

    SECTION("Move constructor") {
        Delta temp;
        temp.add(0, 1, {2});
        Delta d2{std::move(temp)};
        CHECK(d2.contains(0,1,{2}));
    }

    SECTION("Copy assignment") {
        Delta d2;
        d2 = d1;
        CHECK(d2 == d1);
    }

    SECTION("Move assignment") {
        Delta temp;
        temp.add(0, 1, {2});
        Delta d2;
        d2 = std::move(temp);
        CHECK(d2.contains(0,1,{2}));
    }
    SECTION("Get transitions between states") {

        delta.clear();
        delta.add(0, 52, {2, 3});
        delta.add(0, 25, {2, 3});
        delta.add(1, 1, {2});

        auto between = delta.get_transitions_between(0, {2, 3});
        CHECK(between.size() == 2);
        std::set<Symbol> symbols;
        for (const auto& t : between) { symbols.insert(t.symbol); }
        CHECK(symbols == std::set<Symbol>{25, 52});}

    SECTION("Epsilon symbol posts") {

        delta.clear();
        delta.add(0, EPSILON, {1});
        delta.add(0, 5, {2});

        auto it = delta.epsilon_symbol_posts(0);
        CHECK(it != delta.state_post(0).end());
        CHECK(it->symbol == EPSILON);
    }

    SECTION("Defragment removes deleted states and renames correctly") {
        delta.clear();

        delta.add(0, 1, {1});
        delta.add(1, 2, {2});
        delta.add(2, 3, {0});

        BoolVector is_staying{true, false, true};
        std::vector<State> renaming{0, 0, 1};

        delta.defragment(is_staying, renaming);

        CHECK(delta.num_of_states() == 2);

        CHECK_FALSE(delta.contains(0, 1, {1}));
        CHECK(delta.contains(1, 3, {0}));

        // Structural sanity
        CHECK(delta.num_of_transitions() > 0);

        for (const auto& t : delta.get_transitions()) {
            CHECK(t.source < delta.num_of_states());

            for (State target : t.targets) {
                CHECK(target < delta.num_of_states());
            }
        }
    }

    SECTION("Defragment removes transitions containing removed targets") {
        delta.clear();

        delta.add(0, 1, {1,2});
        delta.add(0, 2, {2});

        BoolVector is_staying{true, false, true};
        std::vector<State> renaming{0, 0, 1};

        delta.defragment(is_staying, renaming);

        CHECK_FALSE(delta.contains(0, 1, {0,1}));
        CHECK(delta.contains(0, 2, {1}));

        auto transitions = delta.get_transitions();

        for (const auto& t : transitions) {
            CHECK(t.targets.size() > 0);

            for (State s : t.targets) {
                CHECK(s < delta.num_of_states());
            }
        }
    }

    SECTION("Defragment compacts source states") {
        delta.clear();

        delta.add(0, 1, {1});
        delta.add(2, 2, {0});

        BoolVector is_staying{true, false, true};
        std::vector<State> renaming{0, 0, 1};

        delta.defragment(is_staying, renaming);

        CHECK(delta.num_of_states() == 2);
        CHECK(delta.contains(1, 2, {0}));

        for (const auto& t : delta.get_transitions()) {
            CHECK(t.source < delta.num_of_states());

            for (State target : t.targets) {
                CHECK(target < delta.num_of_states());
            }
        }
    }

    SECTION("Defragment all states removed results in empty delta") {
        delta.clear();

        delta.add(0, 1, {1});
        delta.add(1, 2, {0});

        BoolVector is_staying{false, false};
        std::vector<State> renaming{0,0};

        delta.defragment(is_staying, renaming);

        CHECK(delta.num_of_states() == 0);
        CHECK(delta.is_empty());
    }

    SECTION("Delta is_sorted on empty delta") {
        Delta d;
        CHECK(d.is_sorted());
    }

    SECTION("Delta remains sorted after normal insertions") {
        Delta d;

        d.add(0, 1, {2, 5, 6});
        d.add(0, 2, {3});
        d.add(1, 1, {});

        CHECK(d.is_sorted());
        CHECK(d.num_of_transitions() == 3);
    }

    SECTION("Delta detects duplicate symbol posts") {
        Delta d;

        d.add(0, 5, {2});

        auto& sp = d.mutable_state_post(0); // another symbol post with source 0 symbol 5
        sp.push_back(SymbolPost{5, StateVectorSet{ {2},  {3}}});

        CHECK_FALSE(d.is_sorted());
    }

    SECTION("Delta detects unsorted target tuples") {
        Delta d {1};

        auto& sp = d.mutable_state_post(0);
        StateVectorSet targets;
        targets.push_back({3});
        targets.push_back({2});
        targets.push_back({1});
        sp.push_back(SymbolPost{5, std::move(targets)});

        CHECK_FALSE(d.is_sorted());
    }

    SECTION("Delta detects unsorted SymbolPosts") {
        Delta d;

        auto& sp = d.mutable_state_post(0);

        sp.push_back(SymbolPost{2, StateVectorSet{{1}}});
        sp.push_back(SymbolPost{1, StateVectorSet{{1}}});

        CHECK_FALSE(d.is_sorted());
    }
}


