#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <mata/nfta/nfta.hh>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>
#include <mata/nfta/ranked-alphabet.hh>

using namespace mata::nfta;
using namespace mata::utils;
using namespace mata;

// Count how many target tuples a state has over a given symbol
static size_t count_tuples(const Nfta& aut, State src, Symbol sym) {
    if (src >= aut.delta.num_of_states()) return 0;
    auto it = aut.delta[src].find(SymbolPost{ sym });
    if (it == aut.delta[src].end()) return 0;
    return it->target_tuples.size();
}

// todo classical and det tests
TEST_CASE("mata::nfta::complement_top_down") {

    // todo empty delta, empty initial

    SECTION("Example with parity") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a");   // arity 0
        alphabet.add_new_symbol("s");   // arity 1
        alphabet.add_new_symbol("p");   // arity 2
        Symbol a = alphabet["a"];
        Symbol s = alphabet["s"];
        Symbol p = alphabet["p"];

        Nfta aut({ 0 }, &alphabet, Delta(2));
        aut.delta.add(0, a, {});
        aut.delta.add(0, s, { 1 });
        aut.delta.add(0, p, { 0, 0 });
        aut.delta.add(0, p, { 1, 1 });
        aut.delta.add(1, s, { 0 });
        aut.delta.add(1, p, { 0, 1 });
        aut.delta.add(1, p, { 1, 0 });

        std::unordered_map<StateSet, State> mapping;
        Nfta comp = complement_top_down(aut, &mapping);

        // Identify macrostates by their image in mapping
        REQUIRE(mapping.contains({ 0 }));
        REQUIRE(mapping.contains({ 1 }));
        REQUIRE(mapping.contains({ 0, 1 }));
        REQUIRE(mapping.contains({}));

        State m0  = mapping.at({ 0 });
        State m1  = mapping.at({ 1 });
        State m01 = mapping.at({ 0, 1 });
        State mE  = mapping.at({});

        // initial state {0}
        CHECK(comp.get_initial_states().size() == 1);
        CHECK(comp.is_state_initial(m0));

        // --- {0} transitions ---
        // no leaf transition on a (state 0 accepts a)
        CHECK_FALSE(comp.delta.contains(m0, a, {}));
        // s -> {1}
        CHECK(comp.delta.contains(m0, s, { m1 }));
        // p: exactly the 4 minimal tuples (pruned)
        CHECK(comp.delta.contains(m0, p, { m0,  m1  }));
        CHECK(comp.delta.contains(m0, p, { m1,  m0  }));
        CHECK(comp.delta.contains(m0, p, { m01, mE  }));
        CHECK(comp.delta.contains(m0, p, { mE,  m01 }));
        CHECK(count_tuples(comp, m0, p) == 4);

        // --- {1} transitions ---
        // leaf a accepted (state 1 has no a-transition)
        CHECK(comp.delta.contains(m1, a, {}));
        // s -> {0}
        CHECK(comp.delta.contains(m1, s, { m0 }));
        // p: 4 minimal tuples
        CHECK(comp.delta.contains(m1, p, { m0,  m0  }));
        CHECK(comp.delta.contains(m1, p, { m1,  m1  }));
        CHECK(comp.delta.contains(m1, p, { m01, mE  }));
        CHECK(comp.delta.contains(m1, p, { mE,  m01 }));
        CHECK(count_tuples(comp, m1, p) == 4);

        // --- {0,1} transitions ---
        // no leaf on a
        CHECK_FALSE(comp.delta.contains(m01, a, {}));
        // s -> {0,1}
        CHECK(comp.delta.contains(m01, s, { m01 }));
        // p -> ({0, 1}, {0, 1})
        CHECK(comp.delta.contains(m01, p, { mE,  m01 }));
        CHECK(comp.delta.contains(m01, p, { m01, mE  }));
        CHECK(count_tuples(comp, m01, p) == 2);

        // --- {} (empty) transitions ---
        // leaf a accepted vacuously
        CHECK(comp.delta.contains(mE, a, {}));
        // s -> {}
        CHECK(comp.delta.contains(mE, s, { mE }));
        // p -> ({},{})
        CHECK(comp.delta.contains(mE, p, { mE, mE }));
        CHECK(count_tuples(comp, mE, p) == 1);
    }

    SECTION("Only constant symbol") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a");
        Symbol a = alphabet["a"];

        Nfta aut({ 0 }, &alphabet, Delta(1));
        aut.delta.add(0, a, {});

        std::unordered_map<StateSet, State> mapping;
        Nfta comp = complement_top_down(aut, &mapping);

        REQUIRE(mapping.contains({ 0 }));
        CHECK(mapping.size() == 1);
        State m0 = mapping.at({ 0 });

        // single initial state
        CHECK(comp.get_initial_states().size() == 1);
        CHECK(comp.is_state_initial(m0));

        // {0} does NOT accept a (original does)
        CHECK(comp.delta.empty());
    }

    SECTION("Simple constant and unary") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a");
        alphabet.add_new_symbol("f");
        Symbol a = alphabet["a"];
        Symbol f = alphabet["f"];

        Nfta aut({ 0 }, &alphabet, Delta(2));
        aut.delta.add(0, f, { 1 });
        aut.delta.add(1, f, { 1 });
        aut.delta.add(1, a, {});

        std::unordered_map<StateSet, State> mapping;
        const Nfta comp = complement_top_down(aut, &mapping);

        REQUIRE(mapping.contains({ 0 }));
        REQUIRE(mapping.contains({ 1 }));
        CHECK(mapping.size() == 2);
        State m0 = mapping.at({ 0 });
        State m1 = mapping.at({ 1 });

        // single initial state
        CHECK(comp.get_initial_states().size() == 1);
        CHECK(comp.is_state_initial(m0));

        // {0} accepts a
        CHECK(comp.delta.contains(m0, a, {}));
        // {0} -> f({1})
        CHECK(comp.delta.contains(m0, f, { 1 }));
        // {1} -> f({1})
        CHECK(comp.delta.contains(m1, f, { 1 }));
    }

    SECTION("Unary chain") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a");
        alphabet.add_new_symbol("f");
        Symbol a = alphabet["a"];
        Symbol f = alphabet["f"];

        // 0 -f-> 1 -f-> 0,  only 1 accepts a
        Nfta aut({ 0 }, &alphabet, Delta(2));
        aut.delta.add(0, f, { 1 });
        aut.delta.add(1, f, { 0 });
        aut.delta.add(1, a, {});

        std::unordered_map<StateSet, State> mapping;
        Nfta comp = complement_top_down(aut, &mapping);

        REQUIRE(mapping.contains({ 0 }));
        REQUIRE(mapping.contains({ 1 }));
        State m0 = mapping.at({ 0 });
        State m1 = mapping.at({ 1 });

        // {0} accepts a
        CHECK(comp.delta.contains(m0, a, {}));
        // {1} should NOT accept a
        CHECK_FALSE(comp.delta.contains(m1, a, {}));
        // f-transitions preserved
        CHECK(comp.delta.contains(m0, f, { m1 }));
        CHECK(comp.delta.contains(m1, f, { m0 }));
    }


    SECTION("Two initial states") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a");
        alphabet.add_new_symbol("f");
        Symbol a = alphabet["a"];
        Symbol f = alphabet["f"];

        Nfta aut({ 0, 1 }, &alphabet, Delta(2));
        aut.delta.add(0, f, { 0 });
        aut.delta.add(1, f, { 1 });
        aut.delta.add(0, a, {});
        aut.delta.add(1, a, {});

        std::unordered_map<StateSet, State> mapping;
        Nfta comp = complement_top_down(aut, &mapping);

        // initial macrostate should be {0,1}
        REQUIRE(mapping.contains({ 0, 1 }));
        State m01 = mapping.at({ 0, 1 });
        CHECK(comp.is_state_initial(m01));
        CHECK(comp.get_initial_states().size() == 1);

        // both states accept a, so {0,1} should NOT accept a in complement
        CHECK_FALSE(comp.delta.contains(m01, a, {}));
    }

    SECTION("Binary only") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("p");
        alphabet.add_new_symbol("a");
        Symbol a = alphabet["a"];
        Symbol p = alphabet["p"];

        Nfta aut({ 0 }, &alphabet, Delta(2));
        aut.delta.add(0, p, { 0, 1 });
        aut.delta.add(0, p, { 1, 0 });
        aut.delta.add(1, p, { 0, 0 });
        aut.delta.add(1, a, {});

        std::unordered_map<StateSet, State> mapping;
        Nfta comp = complement_top_down(aut, &mapping);

        REQUIRE(mapping.contains({ 0 }));
        State m0 = mapping.at({ 0 });

        // {0} has no a-transition, so complement accepts a there
        CHECK(comp.delta.contains(m0, a, {}));

        auto it = comp.delta[m0].find(SymbolPost{ p });
        REQUIRE(it != comp.delta[m0].end());
        for (const auto& tup : it->target_tuples) {
            REQUIRE(mapping.count({}));
            State mE = mapping.at({});
            bool both_empty = (tup[0] == mE && tup[1] == mE);
            CHECK(!both_empty);
        }
    }

    SECTION("Double complement") {
        OnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a");
        alphabet.add_new_symbol("s");
        Symbol a = alphabet["a"];
        Symbol s = alphabet["s"];

        Nfta aut({ 0 }, &alphabet, Delta(2));
        aut.delta.add(0, s, { 1 });
        aut.delta.add(1, s, { 0 });
        aut.delta.add(1, a, {});

        std::unordered_map<StateSet, State> mapping1;

        // first complement
        Nfta comp1 = complement_top_down(aut, &mapping1);
        CHECK(mapping1.size() == 2);
        const State m0 = mapping1.at({ 0 });
        const State m1 = mapping1.at({ 1 });
        CHECK(comp1.get_initial_states().size() == 1);
        CHECK(comp1.is_state_initial(m0));

        CHECK(comp1.delta.contains(m0, a, {}));
        CHECK(comp1.delta.contains(m0, s, { 1 }));
        CHECK(comp1.delta.contains(m1, s, { 0 }));

        // complement of a complement
        std::unordered_map<StateSet, State> mapping2;
        Nfta comp2 = complement_top_down(comp1, &mapping2);
        CHECK(comp2.get_initial_states().size() == 1);
        CHECK(comp2.is_state_initial(m0));
        CHECK(comp2.delta.contains(m0, s, { 1 }));
        CHECK(comp2.delta.contains(m1, a, {}));
        CHECK(comp2.delta.contains(m1, s, { 0 }));
        // todo check with equality
    }


    SECTION("Single state and constant, using ranked alphabet") {
        RankedOnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a", 0);
        alphabet.add_new_symbol("f", 1);
        Symbol a = alphabet.translate_symbol("a", 0);
        Symbol f = alphabet.translate_symbol("f", 1);

        Nfta aut({ 0 }, &alphabet, Delta(1));
        aut.delta.add(0, f, { 0 });

        std::unordered_map<StateSet, State> mapping;
        Nfta comp = complement_top_down(aut, &mapping);

        REQUIRE(mapping.count({ 0 }));
        State m0 = mapping.at({ 0 });

        // complement should accept a at {0}
        CHECK(comp.delta.contains(m0, a, {}));
        // f should loop: {0} on f -> ({0})
        CHECK(comp.delta.contains(m0, f, { m0 }));
    }

    SECTION("Complement of universal automaton is empty") {
        IntAlphabet alphabet;
        // one state that accepts everything
        Nfta aut({0}, &alphabet, Delta(1));
        aut.delta.add(0, 0, {});
        aut.delta.add(0, 1, {0});

        Nfta comp = complement_top_down(aut);
        CHECK(comp.is_lang_empty());
    }
    SECTION("Complement of empty-language automaton is non-empty") {
        RankedOnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a", 0);
        alphabet.add_new_symbol("f", 1);
        // state 0 has transitions but never reaches a leaf
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, alphabet.translate_symbol("f", 1), {1});
        aut.delta.add(1, alphabet.translate_symbol("f", 1), {0});

        Nfta comp = complement_top_down(aut);
        CHECK_FALSE(comp.is_lang_empty());
    }
}