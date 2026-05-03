#include <catch2/catch_test_macros.hpp>
#include <mata/nfta/delta.hh>
#include <mata/nfta/types.hh>
#include "mata/alphabet.hh"
#include "mata/nfta/builder.hh"
#include "mata/nfta/nfta.hh"
#include "mata/nfta/ranked-alphabet.hh"

using namespace mata;
using namespace mata::nfta;

TEST_CASE("mata::nfta::is_lang_included") {

    SECTION("Both empty automata") {
        RankedOnTheFlyAlphabet alphabet;
        alphabet.add_new_symbol("a", 0);

        Nfta small_aut({}, &alphabet, Delta(1));
        Nfta big_aut({}, &alphabet, Delta(1));

        // CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::Classical));
        CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::TopDown));
    }

    // SECTION("Empty included in non-empty") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //
    //     Nfta small_aut({}, &alphabet, Delta(1));
    //     Nfta big_aut({0}, &alphabet, Delta(1));
    //     big_aut.delta.add(0, a, {});
    //
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::Classical));
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::TopDown));
    // }

    // SECTION("Non-empty not included in empty") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //
    //     Nfta small_aut({0}, &alphabet, Delta(1));
    //     small_aut.delta.add(0, a, {});
    //     Nfta big_aut({}, &alphabet, Delta(1));
    //
    //     CHECK_FALSE(is_lang_included(small_aut, big_aut, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(small_aut, big_aut, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Same automaton - inclusion holds") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("f", 1);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol f = alphabet.translate_symbol("f", 1);
    //
    //     Nfta aut({0}, &alphabet, Delta(2));
    //     aut.delta.add(0, f, {1});
    //     aut.delta.add(1, a, {});
    //
    //     CHECK(is_lang_included(aut, aut, ComplementMethod::Classical));
    //     CHECK(is_lang_included(aut, aut, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Strict subset") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("f", 1);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol f = alphabet.translate_symbol("f", 1);
    //
    //     // small: accepts only f(a)
    //     Nfta small_aut({0}, &alphabet, Delta(2));
    //     small_aut.delta.add(0, f, {1});
    //     small_aut.delta.add(1, a, {});
    //
    //     // big: accepts f(a) and a
    //     Nfta big_aut({0}, &alphabet, Delta(2));
    //     big_aut.delta.add(0, f, {1});
    //     big_aut.delta.add(0, a, {});
    //     big_aut.delta.add(1, a, {});
    //
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::Classical));
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::TopDown));
    //     CHECK_FALSE(is_lang_included(big_aut, small_aut, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(big_aut, small_aut, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Incomparable languages") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("b", 0);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol b = alphabet.translate_symbol("b", 0);
    //
    //     Nfta aut_a({0}, &alphabet, Delta(1));
    //     aut_a.delta.add(0, a, {});
    //
    //     Nfta aut_b({0}, &alphabet, Delta(1));
    //     aut_b.delta.add(0, b, {});
    //
    //     CHECK_FALSE(is_lang_included(aut_a, aut_b, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(aut_a, aut_b, ComplementMethod::TopDown));
    //     CHECK_FALSE(is_lang_included(aut_b, aut_a, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(aut_b, aut_a, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Universal language contains everything") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("f", 1);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol f = alphabet.translate_symbol("f", 1);
    //
    //     // arbitrary automaton
    //     Nfta small_aut({0}, &alphabet, Delta(2));
    //     small_aut.delta.add(0, f, {1});
    //     small_aut.delta.add(1, a, {});
    //
    //     // universal automaton
    //     Nfta universal({0}, &alphabet, Delta(1));
    //     universal.delta.add(0, a, {});
    //     universal.delta.add(0, f, {0});
    //
    //     CHECK(is_lang_included(small_aut, universal, ComplementMethod::Classical));
    //     CHECK(is_lang_included(small_aut, universal, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Inclusion holds with binary symbol") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("p", 2);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol p = alphabet.translate_symbol("p", 2);
    //
    //     // small: accepts p(a,a) only
    //     Nfta small_aut({0}, &alphabet, Delta(2));
    //     small_aut.delta.add(0, p, {1, 1});
    //     small_aut.delta.add(1, a, {});
    //
    //     // big: accepts any tree with p at root
    //     Nfta big_aut({0}, &alphabet, Delta(2));
    //     big_aut.delta.add(0, p, {1, 1});
    //     big_aut.delta.add(0, p, {0, 1});
    //     big_aut.delta.add(0, p, {1, 0});
    //     big_aut.delta.add(1, a, {});
    //
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::Classical));
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::TopDown));
    //     CHECK_FALSE(is_lang_included(big_aut, small_aut, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(big_aut, small_aut, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Both methods agree on parity example") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("s", 1);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol s = alphabet.translate_symbol("s", 1);
    //
    //     // even parity automaton
    //     Nfta even_aut({0}, &alphabet, Delta(2));
    //     even_aut.delta.add(0, a, {});
    //     even_aut.delta.add(0, s, {1});
    //     even_aut.delta.add(1, s, {0});
    //
    //     // odd parity automaton
    //     Nfta odd_aut({1}, &alphabet, Delta(2));
    //     odd_aut.delta.add(0, a, {});
    //     odd_aut.delta.add(0, s, {1});
    //     odd_aut.delta.add(1, s, {0});
    //
    //     // even and odd are incomparable
    //     CHECK_FALSE(is_lang_included(even_aut, odd_aut, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(even_aut, odd_aut, ComplementMethod::TopDown));
    //     CHECK_FALSE(is_lang_included(odd_aut, even_aut, ComplementMethod::Classical));
    //     CHECK_FALSE(is_lang_included(odd_aut, even_aut, ComplementMethod::TopDown));
    //
    //     // union of even and odd includes each individually
    //     Nfta both({0, 1}, &alphabet, Delta(2));
    //     both.delta.add(0, a, {});
    //     both.delta.add(0, s, {1});
    //     both.delta.add(1, s, {0});
    //
    //     CHECK(is_lang_included(even_aut, both, ComplementMethod::Classical));
    //     CHECK(is_lang_included(even_aut, both, ComplementMethod::TopDown));
    //     CHECK(is_lang_included(odd_aut, both, ComplementMethod::Classical));
    //     CHECK(is_lang_included(odd_aut, both, ComplementMethod::TopDown));
    // }
    //
    // SECTION("Nondeterministic smaller included in deterministic bigger") {
    //     RankedOnTheFlyAlphabet alphabet;
    //     alphabet.add_new_symbol("a", 0);
    //     alphabet.add_new_symbol("f", 1);
    //     Symbol a = alphabet.translate_symbol("a", 0);
    //     Symbol f = alphabet.translate_symbol("f", 1);
    //
    //     // nondeterministic: from 0 on f go to either 1 or 2, both accept a
    //     Nfta small_aut({0}, &alphabet, Delta(3));
    //     small_aut.delta.add(0, f, {1});
    //     small_aut.delta.add(0, f, {2});
    //     small_aut.delta.add(1, a, {});
    //     small_aut.delta.add(2, a, {});
    //
    //     // deterministic: accepts f(a)
    //     Nfta big_aut({0}, &alphabet, Delta(2));
    //     big_aut.delta.add(0, f, {1});
    //     big_aut.delta.add(1, a, {});
    //
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::Classical));
    //     CHECK(is_lang_included(small_aut, big_aut, ComplementMethod::TopDown));
    // }
}
