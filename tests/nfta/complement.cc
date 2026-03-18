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

TEST_CASE("nfta::complement") {

    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("0");
    alphabet.add_new_symbol("s");

    SECTION("Empty") {
        const Nfta aut {};
        Nfta res {};
        CHECK_NOTHROW(res = complement(aut));

        CHECK(res == Nfta());
    }

    SECTION("Simple") {
        Nfta aut {{0}, &alphabet, Delta{2}};

        aut.delta.add(0, alphabet["0"], {});
        aut.delta.add(0, alphabet["s"], {1});
        aut.delta.add(1, alphabet["s"], {0});
        Nfta complem = complement(aut);

        CHECK(complem.delta == aut.delta); // automaton was already deterministic
        CHECK(!complem.is_state_initial(0));
        CHECK(complem.is_state_initial(1));
    }


}