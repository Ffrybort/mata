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

        auto closures = get_epsilon_closures(delta, eps, true);

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

        auto closures = get_epsilon_closures(delta, eps, true);
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

        auto closures1 = get_epsilon_closures(delta, eps, true);
        auto closures2 = get_epsilon_closures(delta, eps, true);

        REQUIRE(closures1 == closures2);
    }

    SECTION("Epsilon cycle") {
        Delta delta;

        delta.add(0, eps, {1});
        delta.add(1, eps, {0});
        delta.add(1, eps, {2});

        auto closures = get_epsilon_closures(delta, eps, true);

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

        auto closures = get_epsilon_closures(delta, eps, true);
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

    SECTION("No reflexivity when include_state is false") {
        Delta delta;

        // 0 -> 1 -> 2
        delta.add(0, eps, {1});
        delta.add(1, eps, {2});

        auto closures = get_epsilon_closures(delta, eps, false);

        REQUIRE(closures[0].count(0) == 0);
        REQUIRE(closures[1].count(1) == 0);
        REQUIRE(closures[2].count(2) == 0);

        REQUIRE(closures[0].contains(1));
        REQUIRE(closures[0].contains(2));

        REQUIRE(closures[1].contains(2));
        REQUIRE(closures[1].size() == 1);

        REQUIRE(closures[2].empty());
    }

    SECTION("Cycle still includes state even if include_state is false") {
        Delta delta;

        // 0 <-> 1, and 1 -> 2
        delta.add(0, eps, {1});
        delta.add(1, eps, {0});
        delta.add(1, eps, {2});

        auto closures = get_epsilon_closures(delta, eps, false);

        REQUIRE(closures[0].contains(1));
        REQUIRE(closures[0].contains(0));
        REQUIRE(closures[0].contains(2));

        REQUIRE(closures[1].contains(0));
        REQUIRE(closures[1].contains(1));
        REQUIRE(closures[1].contains(2));

        REQUIRE(closures[2].empty());
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

        aut = remove_epsilon(aut, eps);

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
        aut = remove_epsilon(aut, eps);
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

        aut = remove_epsilon(aut, eps);

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

TEST_CASE("mata::nfta::remove_epsilon_in_place") {

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

        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon_in_place(eps);

        auto transitions = aut.delta.get_transitions();

        // no epsilon transitions remain
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

        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon_in_place(eps);

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

    SECTION("Epsilon cycle") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 eps q1
            q1 eps q0
            q1 a0 q2
        )";

        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon_in_place(eps);

        auto transitions = aut.delta.get_transitions();

        REQUIRE(std::none_of(transitions.begin(), transitions.end(),
            [eps](const auto& t){ return t.symbol == eps; }));

        // transition from q1 must also appear on q0
        REQUIRE(std::any_of(transitions.begin(), transitions.end(),
            [&](const auto& t){
                return t.source == 0 &&
                       alphabet.reverse_translate_symbol(t.symbol) == "a0";
            }));
    }

    SECTION("Final state propagation") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Final q1
            q0 eps q1
        )";

        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon_in_place(eps);

        // q0 should become final
        REQUIRE(aut.is_state_root(0));
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
            q1 eps q4
            q4 a2 q5
        )";

        Nfta aut = parse_from_mata(input, &alphabet);

        aut.remove_epsilon_in_place(eps);

        auto transitions = aut.delta.get_transitions();

        REQUIRE(std::none_of(transitions.begin(), transitions.end(),
            [eps](const auto& t){ return t.symbol == eps; }));

        // original non-epsilon transition must stay
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

        REQUIRE(std::any_of(transitions.begin(), transitions.end(),
            [&](const auto& t){
                return t.source == 0 &&
                       alphabet.reverse_translate_symbol(t.symbol) == "a2";
            }));
    }
}


