// #include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_vector.hpp>
//
// #include <mata/nfta/nfta.hh>
// #include <mata/nfta/delta.hh>
// #include <mata/nfta/types.hh>
// #include <mata/alphabet.hh>
// #include <mata/nfta/builder.hh>
//
// using namespace mata::nfta;
// using namespace mata::utils;
// using namespace mata;
//
// TEST_CASE("nfta::complement") {
//     // since there is no inclusion or equivalence test yet, complements were computed with the use of Timbuk library
//
//     OnTheFlyAlphabet alphabet;
//     alphabet.add_new_symbol("0");
//     alphabet.add_new_symbol("s");
//
//     SECTION("Empty") {
//         const Nfta aut {};
//         Nfta res {};
//         CHECK_NOTHROW(res = complement(aut));
//
//         CHECK(res == Nfta());
//     }
//
//     SECTION("Simple") {
//         Nfta aut {{0}, &alphabet, Delta{2}};
//
//         aut.delta.add(0, alphabet["0"], {});
//         aut.delta.add(0, alphabet["s"], {1});
//         aut.delta.add(1, alphabet["s"], {0});
//         Nfta aut_compl = complement(aut);
//
//         CHECK(aut_compl.delta == aut.delta); // automaton was already deterministic
//         CHECK(!aut_compl.is_state_initial(0));
//         CHECK(aut_compl.is_state_initial(1));
//     }
//
//     // SECTION("Incomplete") {
//     //     alphabet.clear();
//     //     alphabet.add_new_symbol("a");
//     //     alphabet.add_new_symbol("b");
//     //
//     //     Nfta aut {{1, 2}, &alphabet, Delta{3}};
//     //
//     //     aut.delta.add(0, alphabet["a"], {});
//     //     aut.delta.add(1, alphabet["b"], {0, 0});
//     //     aut.delta.add(2, alphabet["b"], {1, 0});
//     //
//     //     aut.print_readable_bottom_up();
//     //
//     //     aut = complement(aut);
//     //
//     //     CHECK(aut.is_state_initial(0));
//     //     CHECK(!aut.is_state_initial(1));
//     //     CHECK(!aut.is_state_initial(2));
//     //     CHECK(aut.delta.num_of_transitions() == 17);
//     //     CHECK(aut.delta.contains(0, alphabet["a"], {}));
//     //     CHECK(aut.delta.contains(1, alphabet["b"], {0, 0}));
//     //     CHECK(aut.delta.contains(2, alphabet["b"], {1, 0}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {0, 1}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {0, 2}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {0, 3}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {1, 1}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {1, 2}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {1, 3}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {2, 0}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {2, 1}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {2, 2}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {2, 3}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {3, 0}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {3, 1}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {3, 2}));
//     //     CHECK(aut.delta.contains(3, alphabet["b"], {3, 3}));
//     // }
//
//     SECTION("Automaton with a binary symbol") {
//         alphabet.clear();
//         alphabet.add_new_symbol("0");
//         alphabet.add_new_symbol("s");
//         alphabet.add_new_symbol("p");
//
//         Nfta aut {{2}, &alphabet, Delta{2}};
//         aut.delta.add(0, alphabet["0"], {});
//         aut.delta.add(0, alphabet["s"], {1});
//         aut.delta.add(0, alphabet["p"], {0, 0});
//         aut.delta.add(0, alphabet["p"], {1, 1});
//
//         aut.delta.add(1, alphabet["s"], {0});
//         aut.delta.add(1, alphabet["p"], {0, 1});
//         aut.delta.add(1, alphabet["p"], {1, 0});
//
//         aut = complement(aut);
//
//         CHECK(aut.is_state_initial(1));
//         CHECK_FALSE(aut.is_state_initial(0));
//
//         CHECK(aut.delta.contains(1, alphabet["s"], {0}));
//         CHECK(aut.delta.contains(1, alphabet["p"], {0, 1}));
//         CHECK(aut.delta.contains(1, alphabet["p"], {1, 0}));
//         CHECK(aut.delta.contains(0, alphabet["0"], {}));
//         CHECK(aut.delta.contains(0, alphabet["s"], {1}));
//         CHECK(aut.delta.contains(0, alphabet["p"], {0, 0}));
//         CHECK(aut.delta.contains(0, alphabet["p"], {1, 1}));
//         CHECK(aut.delta.num_of_transitions() == 7);
//     }
//
//     SECTION("Nondet automaton") {
//         Nfta aut {{2}, &alphabet, Delta{3}};
//
//         aut = complement(aut);
//
//         CHECK(aut.is_state_initial(1));
//         CHECK(aut.is_state_initial(2));
//         CHECK_FALSE(aut.is_state_initial(0));
//
//         CHECK(aut.delta.contains(0, alphabet["a"], {}));
//         CHECK(aut.delta.contains(1, alphabet["h"], {0,0,0}));
//         CHECK(aut.delta.contains(1, alphabet["h"], {1,0,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,0,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,0,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,1,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,1,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,1,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,2,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,2,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {0,2,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,0,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,0,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,1,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,1,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,1,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,2,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,2,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {1,2,2}));
//
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,0,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,0,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,0,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,1,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,1,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,1,2}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,2,0}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,2,1}));
//         CHECK(aut.delta.contains(2, alphabet["h"], {2,2,2}));
//
//         CHECK(aut.delta.num_of_transitions() == 29);
//     }
//
//
// }