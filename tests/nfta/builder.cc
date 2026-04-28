// testing parse_from_mata

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>
#include <mata/nfta/builder.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

TEST_CASE("mata::nfta::builder") {
    OnTheFlyAlphabet alphabet = OnTheFlyAlphabet();
    IntAlphabet i_alphabet = IntAlphabet();
    RankedOnTheFlyAlphabet ranked_alphabet = RankedOnTheFlyAlphabet();

    SECTION("Create empty") {
        const Nfta aut = create_empty();

        CHECK(aut.root_states.size() == 1);
        CHECK(aut.delta.empty());
        CHECK(aut.delta.num_of_states() == 1);
    }

    SECTION("Create empty") {
        const Nfta aut = create_empty(&alphabet);

        CHECK(aut.root_states.size() == 1);
        CHECK(aut.delta.empty());
        CHECK(aut.delta.num_of_states() == 1);
        CHECK(aut.alphabet == &alphabet);
    }

    SECTION("Create universal empty alphabet") {
        Nfta aut = create_universal(&ranked_alphabet);

        CHECK(aut.root_states.size() == 1);
        CHECK(aut.delta.empty());
        CHECK(aut.delta.num_of_states() == 1);
        CHECK(aut.alphabet == &ranked_alphabet);
        CHECK(ranked_alphabet.empty());
        ranked_alphabet.clear();
    }

    SECTION("Create universal empty symbols") {
        const OrdVector<SymbolArity> symbols;
        Nfta aut = create_universal(&symbols, &alphabet);

        CHECK(aut.root_states.size() == 1);
        CHECK(aut.delta.empty());
        CHECK(aut.delta.num_of_states() == 1);
        CHECK(aut.alphabet == &alphabet);
    }

    SECTION("Create universal ranked alphabet") {
        ranked_alphabet.add_new_symbol("a", 0);
        ranked_alphabet.add_new_symbol("s", 1);
        ranked_alphabet.add_new_symbol("k", 5);

        Nfta aut = create_universal(&ranked_alphabet);

        CHECK(aut.root_states.size() == 1);
        CHECK(aut.delta.num_of_states() == 1);
        CHECK(aut.alphabet == &ranked_alphabet);
        REQUIRE_FALSE(aut.delta.empty());

        CHECK(aut.delta.contains(0, ranked_alphabet.translate_symbol("a", 0), {}));
        CHECK(aut.delta.contains(0, ranked_alphabet.translate_symbol("s", 1), {0}));
        CHECK(aut.delta.contains(0, ranked_alphabet.translate_symbol("k", 5), {0, 0, 0, 0, 0}));

        ranked_alphabet.clear();
    }

    SECTION("Create universal symbols") {
        OrdVector<SymbolArity> symbols = {{0, 0}, {1, 1}};
        Nfta aut = create_universal(&symbols, &i_alphabet);

        CHECK(aut.root_states.size() == 1);
        CHECK(aut.delta.num_of_states() == 1);
        REQUIRE_FALSE(aut.delta.empty());
        CHECK_NOTHROW(i_alphabet["0"]);
        CHECK_NOTHROW(i_alphabet["1"]);

        CHECK(aut.delta.contains(0, i_alphabet["0"], {}));
        CHECK(aut.delta.contains(0, i_alphabet["1"], {0}));
    }

    SECTION("Basic NFTA") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0(2) a1(1) a2
            %Initial q0
            q0 a0 (q1 q2)
            q1 a1 q2
            q2 a2
        )";
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() > 0);
    }

    SECTION("Single target") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0(1) a1(0)
            %Initial q0
            q0 a0 q1
            q1 a1
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() == 2);
    }

    SECTION("Constants") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0 a1 a2
            %Initial q0
            q0 a2
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() == 1);
    }

    SECTION("Many target states") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0(10) a1(2) a2(1) a3
            %Initial q0
            q0 a0 (q1 q2 q3 q1 q2 q3 q1 q2 q3 q15)
            q1 a1 (q4 q5)
            q2 a2 q6
            q3 a3
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() == 8);
    }


    SECTION("Single symbol alphabet-auto") {
        std::string input = R"(
            @NFTA-explicit
            %States-enum q0
            %Alphabet-auto
            %Initial q0
            q0 a0 ()
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() == 1);
        CHECK(aut.root_states.size() == 1);
    }

    SECTION("Alphabet-marked, states-marked") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-marked
            %Initial q0 q1
            q0 a0 (q2 q3)
            q1 a1 q3
            q2 a0 (q0 q1)
            q3 a1 q2
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.root_states.size() == 2);
    }

    SECTION("Multiple transitions from same source and symbol 1") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0(2) a1(1) a2
            %Initial q0
            q0 a0 (q1 q2)
            q0 a0 (q2 q3)
            q1 a1 q3
            q2 a2
            q3 a1 q0
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        std::vector<Transition> t = aut.delta.get_transitions();
        CHECK(std::count_if(
            t.begin(), t.end(),
            [alphabet](const auto& t){ return t.single == 0 && alphabet.reverse_translate_symbol(t.symbol) == "a0"; }
        ) == 2);
    }


    SECTION("Multiple transitions from same source and symbol 2") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-marked
            %Initial q0
            q0 a0 (q1 q2)
            q0 a0 (q2 q3)
            q1 a1 q3
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        const StatePost& sp = aut.delta.state_post(0);
        auto moves = sp.moves();
        std::set<std::vector<State>> targets_found;
        for (auto it = moves.begin(); it != StatePost::Moves::end(); ++it) {
            if (alphabet.reverse_translate_symbol(it->symbol) == "0") {
                targets_found.insert(it->targets);
            }
        }
        CHECK(targets_found.count({1,2}) == 1); // q1 q2
        CHECK(targets_found.count({2,3}) == 1); // q2 q3
        CHECK(targets_found.size() == 2);
    }

    SECTION("Cycle") {
        std::string input = R"(
        @NFTA-explicit
        %States-marked
        %Alphabet-auto
        %Initial q0
        q0 a0 q1
        q1 a1 q0
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() == 2);
    }

    SECTION("Single state loop") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-marked
            %Initial q0
            q0 a0 (q0)
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() == 1);
    }

    SECTION("One big boi") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0(3) a1(2) a2(1) a3(0)
            %Initial q0 q1 q2
            q0 a0  (q3 q4 q5)
            q1 a0  (q6 q7 q8)
            q2 a1  (q9 q10)
            q3 a1  (q11 q12)
            q4 a2  (q13)
            q5 a3
            q6 a0  (q14 q15 q16)
            q7 a1  (q17 q18)
            q8 a2  (q19)
            q9 a0  (q20 q21 q22)
            q10 a1 (q23 q24)
            q11 a2 (q25)
            q12 a3
            q13 a0 (q26 q27 q28)
            q14 a1 (q29 q30)
            q15 a2 (q31)
            q16 a3
            q17 a0 (q32 q33 q34)
            q18 a1 (q35 q36)
            q19 a2 (q37)
            q20 a0 (q38 q39 q40)
            q21 a1 (q41 q42)
            q22 a2 (q43)
            q23 a3
            q24 a0 (q44 q45 q46)
            q25 a1 (q47 q48)
            q26 a2 (q49)
            q27 a3
            q28 a0 (q50 q51 q52)
            q29 a1 (q53 q54)
            q30 a2 (q55)
            q31 a3
            q32 a0 (q56 q57 q58)
            q33 a1 (q59 q60)
            q34 a2 (q61)
            q35 a3
            q36 a0 (q62 q63 q64)
            q37 a1 (q65 q66)
            q38 a2 (q67)
            q39 a3
            q40 a0 (q68 q69 q70)
            q41 a1 (q71 q72)
            q42 a2 (q73)
            q43 a3
            q44 a0 (q74 q75 q76)
            q45 a1 (q77 q78)
            q46 a2 (q79)
            q47 a3
            q48 a0 (q80 q81 q82)
            q49 a1 (q83 q84)
            q50 a2 (q85)
            q51 a3
            q52 a0 (q86 q87 q88)
            q53 a1 (q89 q90)
            q54 a2 (q91)
            q55 a3
            q56 a0 (q92 q93 q94)
            q57 a1 (q95 q96)
            q58 a2 (q97)
            q59 a3
            q60 a0 (q98 q99 q100)
            q61 a1 (q101 q102)
            q62 a2 (q103)
            q63 a3
            q64 a0 (q0 q1 q2)
        )";
        alphabet.clear();
        Nfta aut = parse_from_mata(input, &alphabet);
        CHECK(aut.delta.num_of_states() >= 104);
        CHECK(aut.root_states.size() == 3);
    }

    SECTION("Single state using IntAlphabet") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum 0(1)
            %Initial q0
            q0 0 q0
        )";
        Nfta aut = parse_from_mata(input, &i_alphabet);
        CHECK(aut.delta.num_of_states() == 1);
    }

    SECTION("Parse^2") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 0 (q1 q2)
            q1 1 q3
            q2 2
            q3 2
        )";
        Nfta aut1 = parse_from_mata(input, &alphabet);

        // print automaton to string
        std::ostringstream out;
        CHECK_NOTHROW(aut1.print_mata(out));
        std::string printed = out.str();
        alphabet.clear();

        Nfta aut2 = parse_from_mata(printed, &alphabet);

        bool check = aut1.is_identical_to(aut2);
        CHECK(check);
    }

    SECTION("Ranked alphabet basic usage (enum symbols)") {
        ranked_alphabet.clear();

        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a0(2) a1(1)
            %Initial q0
            q0 a0 (q1 q2)
            q1 a1 q2
        )";

        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        CHECK(aut.delta.num_of_states() == 3);
        CHECK(ranked_alphabet.get_alphabet_symbols().size() == 2);
    }

    SECTION("Ranked alphabet auto symbols") {
        ranked_alphabet.clear();

        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 f (q1 q2 q3)
            q1 g q2
            q2 h
        )";

        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        CHECK(aut.delta.num_of_states() == 4);
        CHECK(aut.delta.num_of_transitions() == 3);
        CHECK(ranked_alphabet.get_alphabet_symbols().size() == 3);

        // arities inferred from transitions
        CHECK(ranked_alphabet.get_arity(
            ranked_alphabet.translate_symbol("f", 3)
        ) == std::vector<unsigned>{3});
    }

    SECTION("Ranked alphabet constant symbols") {
        ranked_alphabet.clear();

        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 c
        )";

        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        CHECK(aut.delta.num_of_states() == 1);
        CHECK(aut.delta.num_of_transitions() == 1);
        CHECK(ranked_alphabet.get_alphabet_symbols().size() == 1);

        Symbol c = ranked_alphabet.translate_symbol("c", 0);
        CHECK(ranked_alphabet.get_arity(c) == std::vector<unsigned>{0});
    }

    SECTION("Ranked alphabet: same symbol with same arity multiple times") {
        ranked_alphabet.clear();

        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 f (q1 q2)
            q1 f (q2 q3)
            q2 f (q3 q4)
        )";

        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        CHECK(aut.delta.num_of_states() == 5);
        CHECK(aut.delta.num_of_transitions() == 3);
        CHECK(aut.delta.contains(0, alphabet["f"], {1, 2}));
        CHECK(aut.delta.contains(1, alphabet["f"], {2, 3}));
        CHECK(aut.delta.contains(2, alphabet["f"], {3, 4}));
        CHECK(ranked_alphabet.get_alphabet_symbols().size() == 1);
    }

    SECTION("Ranked alphabet: same symbol with different arities (overload)") {
        ranked_alphabet.clear();

        std::string input = R"(
            @NFTA-explicit
            %Overload
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 f (q1)
            q1 f (q2 q3)
            q2 f
        )";

        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        CHECK(aut.delta.num_of_states() == 4);
        CHECK(aut.delta.num_of_transitions() == 3);

        // same name, different arities → multiple ranked symbols
        CHECK(ranked_alphabet.get_alphabet_symbols().size() == 3);
    }

    SECTION("Ranked alphabet enum: arity mismatch throws") {
        ranked_alphabet.clear();

        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum f(2)
            %Initial q0
            q0 f q1
        )";

        CHECK_THROWS_AS(
            parse_from_mata(input, &ranked_alphabet),
            std::runtime_error
        );
    }

    SECTION("Ranked alphabet consistency with generic alphabet") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 f (q1 q2)
            q1 g q2
            q2 h
        )";

        ranked_alphabet.clear();
        alphabet.clear();

        Nfta ranked_aut = parse_from_mata(input, &ranked_alphabet);
        Nfta generic_aut = parse_from_mata(input, &alphabet);

        CHECK(ranked_aut.delta.num_of_states() == generic_aut.delta.num_of_states());
        CHECK(ranked_aut.root_states == generic_aut.root_states);
        CHECK(ranked_aut.delta == generic_aut.delta);
    }

    SECTION("Overload alphabet enumerated") {
        std::string input = R"(
            @NFTA-explicit
            %Overload
            %States-marked
            %Alphabet-enum a(1) a(2)
            %Initial q0
            q0 a q1
            q0 a (q1 q2)
        )";
        ranked_alphabet.clear();
        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        auto symbols = ranked_alphabet.get_alphabet_symbols();
        CHECK(symbols.size() == 2);
    }

    SECTION("Overload alphabet auto") {
        std::string input = R"(
            @NFTA-explicit
            %Overload
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 a q1
            q0 a (q1 q2 q3)
        )";
        ranked_alphabet.clear();
        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        CHECK(aut.delta.num_of_states() == 4);
    }

    SECTION("Overload enumerated alphabet not defined - fails") {
        std::string input = R"(
            @NFTA-explicit
            %Overload
            %States-marked
            %Alphabet-enum a(1)
            %Initial q0
            q0 a (q1 q2)
        )";
        ranked_alphabet.clear();
        CHECK_THROWS(parse_from_mata(input, &ranked_alphabet));
    }

    SECTION("Overload auto alphabet – multiple overloaded transitions") {
        std::string input = R"(
            @NFTA-explicit
            %Overload
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 a q1
            q0 a (q1 q2)
            q0 a (q2 q1)
            q0 a (q2 q3 q4)
        )";
        ranked_alphabet.clear();
        Nfta aut = parse_from_mata(input, &ranked_alphabet);

        auto transitions = aut.delta.get_transitions();
        CHECK(transitions.size() == 4);
        CHECK(ranked_alphabet.get_alphabet_symbols().size() == 3); // 3 different arities
    }

    SECTION("No overload auto – same symbol name with different arities fails") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-auto
            %Initial q0
            q0 a q1
            q0 a (q1 q2)
        )";
        ranked_alphabet.clear();
        CHECK_THROWS(parse_from_mata(input, &ranked_alphabet));
    }

    SECTION("No overload enumerated – same symbol name with different arities fails") {
        std::string input = R"(
            @NFTA-explicit
            %States-marked
            %Alphabet-enum a(1) a(2)
            %Initial q0
            q0 a q1
            q0 a (q1 q2)
        )";
        ranked_alphabet.clear();
        CHECK_THROWS(parse_from_mata(input, &ranked_alphabet));
    }
}
