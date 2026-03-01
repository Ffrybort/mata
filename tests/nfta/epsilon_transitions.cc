// testing epsilon-related operations

#include <catch2/catch_test_macros.hpp>
#include "mata/nfta/nfta.hh"
#include "mata/nfta/builder.hh"
#include "mata/alphabet.hh"

using namespace mata::nfta;
using namespace mata;

TEST_CASE("mata::nfta::get_epsilon_closures") {

    Symbol eps = 0;

    SECTION("Reflexivity") {
        Delta delta;

        delta.add(0, eps, {1});
        delta.add(1, eps, {2});
        delta.add(2, eps, {3});

        auto closures = get_epsilon_closures(delta, eps);

        // Every state must contain itself
        for (State s = 0; s < static_cast<State>(closures.size()); s++) {
            REQUIRE(closures[s].count(s) == 1);
        }
    }

    SECTION("Transitive epsilon chain") {
        Delta delta;

        // 0 -> 1 -> 2 -> 3
        delta.add(0, eps, {1});
        delta.add(1, eps, {2});
        delta.add(2, eps, {3});

        auto closures = get_epsilon_closures(delta, eps);
        auto& c0 = closures[0];

        REQUIRE(c0.contains(1));
        REQUIRE(c0.contains(2));
        REQUIRE(c0.contains(3));

        REQUIRE(c0.size() == 4); // {0,1,2,3}
    }

    SECTION("Nothing is changed") {
        Delta delta;

        delta.add(0, eps, {1});
        delta.add(1, eps, {2});
        delta.add(2, eps, {3});

        auto closures1 = get_epsilon_closures(delta, eps);
        auto closures2 = get_epsilon_closures(delta, eps);

        REQUIRE(closures1 == closures2);
    }

    SECTION("Epsilon cycle") {
        Delta delta;

        delta.add(0, eps, {1});
        delta.add(1, eps, {0});
        delta.add(1, eps, {2});

        auto closures = get_epsilon_closures(delta, eps);

        REQUIRE(closures[0].count(2));
        REQUIRE(closures[1].count(2));
    }
    SECTION("Complex graph correctness") {
        Delta delta;
        /*
            0 -> 1 -> 2 -> 3
            0 -> 4
            4 -> 5 -> 6
            2 -> 7
        */

        delta.add(0, eps, {1});
        delta.add(1, eps, {2});
        delta.add(2, eps, {3});
        delta.add(0, eps, {4});
        delta.add(4, eps, {5});
        delta.add(5, eps, {6});
        delta.add(2, eps, {7});

        auto closures = get_epsilon_closures(delta, eps);
        REQUIRE(closures.size() == 8);
        auto check_set = [](const StateSet& set,
                            const std::vector<State>& expected) {
            REQUIRE(set.size() == expected.size());
            for (auto s : expected) {
                REQUIRE(set.count(s) == 1);
            }
        };

        check_set(closures[0], {0,1,2,3,4,5,6,7});
        check_set(closures[1], {1,2,3,7});
        check_set(closures[2], {2,3,7});
        check_set(closures[3], {3});
        check_set(closures[4], {4,5,6});
        check_set(closures[5], {5,6});
        check_set(closures[6], {6});
        check_set(closures[7], {7});
    }
}

TEST_CASE("mata::nfta::remove_epsilon") {
    Delta delta;
    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("eps");
    Symbol eps = alphabet.translate_symb("eps");

    SECTION("Single epsilon transition") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 eps q1
            q1 a0 q2
        )";

        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon(eps);

        auto transitions = aut.delta.get_transitions();

        REQUIRE(std::none_of(transitions.begin(), transitions.end(),
            [eps](const auto& t){ return t.symbol == eps; }));

        REQUIRE(std::any_of(transitions.begin(), transitions.end(),
            [&](const auto& t){
                return t.source == 0 &&
                       alphabet.reverse_translate_symbol(t.symbol) == "a0";
            }));
    }


    SECTION("Multiple epsilon transitions from same source") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 eps q1
            q0 eps (q2)
            q1 a0 q3
            q2 a0 q4
        )";

        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        aut.remove_epsilon(eps);
        auto transitions = aut.delta.get_transitions();

        REQUIRE(std::none_of(transitions.begin(), transitions.end(),
            [eps](const auto& t){ return t.symbol == eps; }));

        auto count = std::ranges::count_if(transitions,
            [&](const auto& t){
                return t.source == 0 &&
                       alphabet.reverse_translate_symbol(t.symbol) == "a0";
            });

        REQUIRE(count == 2);
    }


    SECTION("Mixed epsilon and normal transitions") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 eps q1
            q0 a1 q2
            q1 a0 q3
        )";

        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon(eps);

        auto transitions = aut.delta.get_transitions();

        REQUIRE(std::any_of(transitions.begin(), transitions.end(),
            [&](const auto& t){
                return t.source == 0 &&
                       alphabet.reverse_translate_symbol(t.symbol) == "a1";
            }));

        REQUIRE(std::any_of(transitions.begin(), transitions.end(),
            [&](const auto& t){
                return t.source == 0 &&
                       alphabet.reverse_translate_symbol(t.symbol) == "a0";
            }));
    }
}

