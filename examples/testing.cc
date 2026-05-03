/**
 * Simple main function for manual debugging purposes.
 */

#include <fstream>
#include <iostream>
#include <sstream>

#include <filesystem>
#include <mata/alphabet.hh>
#include <mata/nfta/builder.hh>
#include <mata/nfta/nfta.hh>
#include <mata/parser/inter-aut.hh>
#include <mata/parser/parser.hh>
#include <mata/utils/utils.hh>

#include "../tests-integration/src/utils/utils.hh"


using namespace mata;
using namespace mata::nfta;
using namespace mata::utils;

void print_nfta_eq_debug(const Nfta& aut1, const Nfta& aut2, std::ostream& os) {
    const bool states_eq = aut1.delta.num_of_states() == aut2.delta.num_of_states();

    const bool finals_eq = aut1.root_states == aut2.root_states;

    const bool delta_eq = aut1.delta == aut2.delta;

    const bool alphabet_eq = (aut1.alphabet == aut2.alphabet) ||
                             (aut1.alphabet && aut2.alphabet && aut1.alphabet->is_equal(aut2.alphabet));

    os << std::boolalpha;
    os << "Nfta equality debug:\n";
    os << "  num_of_states: " << states_eq << "\n";
    os << "  root_states:  " << finals_eq << "\n";
    os << "  delta:         " << delta_eq << "\n";
    os << "  alphabet:      " << alphabet_eq << "\n";
}

void print_vec2d(const std::vector<StateSet>& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "[" << i << "] : ";
        for (const auto& elem : v[i]) {
            std::cout << elem << " ";
        }
        std::cout << '\n';
    }
}

template<typename T>
void debug_print_product_transitions(
        const mata::nfta::Nfta& product, const mata::utils::TwoDimensionalMap<T>& mapping, size_t A_states,
        size_t B_states, const Alphabet* alphabet, std::ostream& os = std::cout) {
    using namespace mata::nfta;

    const T INF = std::numeric_limits<T>::max();

    const auto& transitions = product.delta.get_transitions();

    for (const auto& tr : transitions) {

        State product_source = tr.single;

        // source pair
        State source_A = mapping.get_first_inverted(product_source);
        State source_B = mapping.get_second_inverted(product_source);

        os << "(" << source_A << "," << source_B << ") -> ";

        if (alphabet) {
            os << alphabet->reverse_translate_symbol(tr.symbol);
        } else {
            os << tr.symbol;
        }

        if (!tr.tuple.empty()) {
            os << " (";

            // print each target as pair
            for (size_t i = 0; i < tr.tuple.size(); ++i) {
                if (i > 0)
                    os << ", ";
                State tgt = tr.tuple[i];
                State tgt_A = mapping.get_first_inverted(tgt);
                State tgt_B = mapping.get_second_inverted(tgt);
                os << "(" << tgt_A << "," << tgt_B << ")";
            }

            os << ")";
        }

        os << "\n";
    }
}
template<typename T>
void debug_print_two_dim_map(const mata::utils::TwoDimensionalMap<T>& map, T first_dim, T second_dim) {
    for (T i = 0; i < first_dim; ++i) {
        for (T j = 0; j < second_dim; ++j) {
            T val = map.get(i, j);
            if (val != std::numeric_limits<T>::max()) {
                std::cout << "(" << i << ", " << j << ") -> " << val << "\n";
            }
        }
    }
}

void REQUIRE(bool cond) { std::cout << "REQUIRE(" << cond << ")" << std::endl; }

void CHECK(bool cond) { std::cout << "CHECK(" << cond << ")" << std::endl; }

void CHECK_FALSE(bool cond) { std::cout << "CHECK_FALSE(" << cond << ")" << std::endl; }

void print_bool_vector(const BoolVector& bv) {
    std::cout << "[";
    for (size_t j = 0; j < bv.size(); ++j) {
        if (j)
            std::cout << ", ";
        std::cout << static_cast<int>(bv[j]);
    }
    std::cout << "]\n";
}

void print_state_mapping(const std::unordered_map<StateSet, State>& mapping) {
    std::cout << "State mapping:\n";

    for (const auto& [stateset, state] : mapping) {
        std::cout << "{";

        for (size_t i = 0; i < stateset.size(); ++i) {
            std::cout << stateset.at(i);
            if (i + 1 < stateset.size()) {
                std::cout << ", ";
            }
        }

        std::cout << "} -> " << state << "\n";
    }

    std::cout << std::endl;
}

// Check that a specific transition tuple exists in the result
static bool has_targets(const Nfta& aut, State src, Symbol sym, const std::vector<State>& tup) {
    return aut.delta.contains(src, sym, tup);
}

// Check that a specific transition tuple does NOT exist
static bool not_has_tuple(const Nfta& aut, State src, Symbol sym, const std::vector<State>& tup) {
    return !aut.delta.contains(src, sym, tup);
}

// Count how many target tuples a state has over a given symbol
static size_t count_tuples(const Nfta& aut, State src, Symbol sym) {
    if (src >= aut.delta.num_of_states())
        return 0;
    auto it = aut.delta[src].find(SymbolPost{sym});
    if (it == aut.delta[src].end())
        return 0;
    return it->target_tuples.size();
}

void print_macrostates(const Nfta& comp, const std::unordered_map<StateSet, State>& mapping) {
    // invert mapping: State -> StateSet
    std::vector<StateSet> state_to_macro(comp.delta.num_of_states());
    for (const auto& [ss, s] : mapping)
        state_to_macro[s] = ss;

    auto print_stateset = [](const StateSet& ss) {
        std::cout << "{";
        bool first = true;
        for (const State q : ss) {
            if (!first)
                std::cout << ", ";
            std::cout << q;
            first = false;
        }
        std::cout << "}";
    };

    for (State src = 0; src < comp.delta.num_of_states(); ++src) {
        for (const auto& symbol_post : comp.delta[src]) {
            std::string sym_name = comp.alphabet ? comp.alphabet->reverse_translate_symbol(symbol_post.symbol)
                                                 : std::to_string(symbol_post.symbol);

            if (symbol_post.target_tuples.empty()) {
                // leaf
                print_stateset(state_to_macro[src]);
                std::cout << " -> " << sym_name << "\n";
            } else {
                for (const auto& tup : symbol_post.target_tuples) {
                    print_stateset(state_to_macro[src]);
                    std::cout << " -> " << sym_name << "(";
                    for (size_t i = 0; i < tup.size(); ++i) {
                        if (i)
                            std::cout << ", ";
                        print_stateset(state_to_macro[tup[i]]);
                    }
                    std::cout << ")\n";
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    //     if (argc != 2) {
    //         std::cerr << "Input files missing\n";
    //         return EXIT_FAILURE;
    //     }
    //
    //     mata::nfta::RankedOnTheFlyAlphabet alphabet;
    //     std::ifstream file_1(argv[1]);
    //     if (!file_1.is_open()) {
    //         std::cout << "could not open file \n";
    //         return EXIT_FAILURE;
    //     }
    //
    //     Nfta aut = parse_from_mata(file_1, &alphabet);
    //     file_1.close();
    //     TIME_BEGIN(res);
    // Nfta result = complement_top_down(aut);
    // TIME_END(res);
    //
    // return EXIT_SUCCESS;

    OnTheFlyAlphabet alphabet;
    const ParameterMap naive_params = {{"algorithm", "naive"}};


    Nfta aut({}, &alphabet, Delta(2));

    aut.delta.add(0, alphabet["a"], {});
    aut.delta.add(1, alphabet["a"], {});

    std::unordered_map<StateSet, State> mapping;

    Nfta aut_n = determinize(aut, naive_params);
    Nfta aut_0 = determinize_optimized(aut);
    CHECK(aut_n.is_identical_to(aut_0));

    CHECK(aut_n.is_bottom_up_deterministic());

    // expect macrostate {0,1}
    bool found = false;

    for (const auto& [set, det_state] : mapping) {
        if (set.size() == 2 && set.at(0) == 0 && set.at(1) == 1) {
            found = true;
        }
    }
    CHECK(found);
}
