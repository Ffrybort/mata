/**
* Simple main function for manual debugging purposes.
*/

#include <fstream>
#include <iostream>
#include <sstream>

#include <mata/alphabet.hh>
#include <mata/utils/utils.hh>
#include <mata/nfta/nfta.hh>
#include <mata/parser/parser.hh>
#include <mata/parser/inter-aut.hh>
#include <mata/nfta/builder.hh>

using namespace mata;
using namespace mata::nfta;
using namespace mata::utils;



void print_nfta_eq_debug(const Nfta& aut1, const Nfta& aut2, std::ostream& os)
{
    const bool states_eq =
        aut1.delta.num_of_states() == aut2.delta.num_of_states();

    const bool finals_eq =
        aut1.initial_states == aut2.initial_states;

    const bool delta_eq =
        aut1.delta == aut2.delta;

    const bool alphabet_eq =
        (aut1.alphabet == aut2.alphabet) ||
        (aut1.alphabet && aut2.alphabet &&
         aut1.alphabet->is_equal(aut2.alphabet));

    os << std::boolalpha;
    os << "Nfta equality debug:\n";
    os << "  num_of_states: " << states_eq << "\n";
    os << "  initial_states:  " << finals_eq << "\n";
    os << "  delta:         " << delta_eq << "\n";
    os << "  alphabet:      " << alphabet_eq << "\n";
}

void print_vec2d(const std::vector<StateSet>& v)
{
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
    const mata::nfta::Nfta& product,
    const mata::utils::TwoDimensionalMap<T>& mapping,
    size_t A_states,
    size_t B_states,
    const Alphabet* alphabet,
    std::ostream& os = std::cout)
{
    using namespace mata::nfta;

    const T INF = std::numeric_limits<T>::max();

    const auto& transitions = product.delta.get_transitions();

    for (const auto& tr : transitions) {

        State product_source = tr.source;

        // source pair
        State source_A = mapping.get_first_inverted(product_source);
        State source_B = mapping.get_second_inverted(product_source);

        os << "(" << source_A << "," << source_B << ") -> ";

        if (alphabet) {
            os << alphabet->reverse_translate_symbol(tr.symbol);
        } else {
            os << tr.symbol;
        }

        if (!tr.targets.empty()) {
            os << " (";

            // print each target as pair
            for (size_t i = 0; i < tr.targets.size(); ++i) {
                if (i > 0) os << ", ";
                State tgt = tr.targets[i];
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
void debug_print_two_dim_map(const mata::utils::TwoDimensionalMap<T>& map,
                             T first_dim,
                             T second_dim)
{
    for (T i = 0; i < first_dim; ++i) {
        for (T j = 0; j < second_dim; ++j) {
            T val = map.get(i, j);
            if (val != std::numeric_limits<T>::max()) {
                std::cout << "(" << i << ", " << j << ") -> " << val << "\n";
            }
        }
    }
}

void REQUIRE(bool cond) {
    std::cout << "REQUIRE(" << cond << ")" << std::endl;
}

void CHECK(bool cond) {
    std::cout << "CHECK(" << cond << ")" << std::endl;
}

void CHECK_FALSE(bool cond) {
    std::cout << "CHECK_FALSE(" << cond << ")" << std::endl;
}

void print_bool_vector(const BoolVector& bv) {
    std::cout << "[";
    for (size_t j = 0; j < bv.size(); ++j) {
        if (j) std::cout << ", ";
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
static bool has_targets(const Nfta& aut, State src, Symbol sym,
                      const std::vector<State>& tup) {
    return aut.delta.contains(src, sym, tup);
}

// Check that a specific transition tuple does NOT exist
static bool not_has_tuple(const Nfta& aut, State src, Symbol sym,
                           const std::vector<State>& tup) {
    return !aut.delta.contains(src, sym, tup);
}

// Count how many target tuples a state has over a given symbol
static size_t count_tuples(const Nfta& aut, State src, Symbol sym) {
    if (src >= aut.delta.num_of_states()) return 0;
    auto it = aut.delta[src].find(SymbolPost{ sym });
    if (it == aut.delta[src].end()) return 0;
    return it->target_tuples.size();
}

void print_macrostates(
    const Nfta& comp,
    const std::unordered_map<StateSet, State>& mapping)
{
    // invert mapping: State -> StateSet
    std::vector<StateSet> state_to_macro(comp.delta.num_of_states());
    for (const auto& [ss, s] : mapping)
        state_to_macro[s] = ss;

    auto print_stateset = [](const StateSet& ss) {
        std::cout << "{";
        bool first = true;
        for (const State q : ss) {
            if (!first) std::cout << ", ";
            std::cout << q;
            first = false;
        }
        std::cout << "}";
    };

    for (State src = 0; src < comp.delta.num_of_states(); ++src) {
        for (const auto& symbol_post : comp.delta[src]) {
            std::string sym_name = comp.alphabet
                ? comp.alphabet->reverse_translate_symbol(symbol_post.symbol)
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
                        if (i) std::cout << ", ";
                        print_stateset(state_to_macro[tup[i]]);
                    }
                    std::cout << ")\n";
                }
            }
        }
    }
}

int main() {

    OnTheFlyAlphabet alphabet;
    alphabet.add_new_symbol("a");
    alphabet.add_new_symbol("f");
    alphabet.add_new_symbol("p");
    alphabet.add_new_symbol("b");
    alphabet.add_new_symbol("g");

    Symbol a = alphabet["a"];
    Symbol f = alphabet["f"];
    Symbol p = alphabet["p"];
    Symbol b = alphabet["b"];
    Symbol g = alphabet["g"];

    // 1 Empty automaton
    {
        Nfta aut;
        aut.print_timbuk();
    }

    // 2 Empty delta
    {
        Nfta aut({5}, &alphabet, Delta(0));
        aut.print_timbuk();
    }

    // 3 No initial states
    {
        Nfta aut({}, &alphabet, Delta(1));
        aut.delta.add(0, a, {});
        aut.print_timbuk();
    }

    // 4 Single state accepts constant
    {
        Nfta aut({0}, &alphabet, Delta(1));
        aut.delta.add(0, a, {});
        aut.print_timbuk();
    }

    // 5 Reachable leaf via unary symbol
    {
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, f, {1});
        aut.delta.add(1, a, {});
        aut.print_timbuk();
    }

    // 6 No reachable leaf
    {
        Nfta aut({0}, &alphabet, Delta(3));
        aut.delta.add(0, f, {1});
        aut.delta.add(1, f, {0});
        aut.delta.add(2, a, {});
        aut.print_timbuk();
    }

    // 7 Reachable leaf via binary symbol
    {
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, p, {1, 1});
        aut.delta.add(1, a, {});
        aut.print_timbuk();
    }

    // 8 Binary symbol, no leaf
    {
        Nfta aut({0}, &alphabet, Delta(2));
        aut.delta.add(0, p, {1, 1});
        aut.delta.add(1, p, {0, 0});
        aut.print_timbuk();
    }

    // 9 Multiple initial states, one reaches leaf
    {
        Nfta aut({0, 1}, &alphabet, Delta(3));
        aut.delta.add(0, f, {2});
        aut.delta.add(1, a, {});
        aut.print_timbuk();
    }

    // 10 Multiple initial states, none reach leaf
    {
        Nfta aut({0, 1}, &alphabet, Delta(5));
        aut.delta.add(0, f, {1});
        aut.delta.add(1, f, {0});
        aut.delta.add(2, a, {});
        aut.delta.add(3, a, {});
        aut.delta.add(4, a, {});
        aut.print_timbuk();
    }

    // 11 Initial state is also a leaf state
    {
        Nfta aut({0}, &alphabet, Delta(1));
        aut.delta.add(0, a, {});
        aut.delta.add(0, f, {0});
        aut.print_timbuk();
    }

    // 12 Large automaton — empty language
    {
        Nfta aut({0}, &alphabet, Delta(8));

        aut.delta.add(0, f, {1});
        aut.delta.add(0, g, {2});
        aut.delta.add(0, p, {1, 2});
        aut.delta.add(0, p, {2, 1});
        aut.delta.add(0, p, {3, 3});

        aut.delta.add(1, f, {2});
        aut.delta.add(1, g, {3});
        aut.delta.add(1, p, {0, 2});
        aut.delta.add(1, p, {2, 0});
        aut.delta.add(1, p, {4, 4});

        aut.delta.add(2, f, {3});
        aut.delta.add(2, g, {0});
        aut.delta.add(2, p, {1, 3});
        aut.delta.add(2, p, {3, 1});
        aut.delta.add(2, p, {0, 4});

        aut.delta.add(3, f, {4});
        aut.delta.add(3, g, {1});
        aut.delta.add(3, p, {0, 1});
        aut.delta.add(3, p, {2, 4});
        aut.delta.add(3, p, {4, 0});

        aut.delta.add(4, f, {0});
        aut.delta.add(4, g, {2});
        aut.delta.add(4, p, {1, 4});
        aut.delta.add(4, p, {3, 0});
        aut.delta.add(4, p, {2, 2});

        aut.delta.add(5, a, {});
        aut.delta.add(5, b, {});
        aut.delta.add(5, f, {6});
        aut.delta.add(5, g, {7});
        aut.delta.add(5, p, {6, 7});

        aut.delta.add(6, a, {});
        aut.delta.add(6, f, {5});
        aut.delta.add(6, g, {7});
        aut.delta.add(6, p, {5, 7});
        aut.delta.add(6, p, {7, 5});

        aut.delta.add(7, b, {});
        aut.delta.add(7, f, {6});
        aut.delta.add(7, g, {5});
        aut.delta.add(7, p, {5, 5});
        aut.delta.add(7, p, {6, 6});

        aut.print_timbuk();
    }

    return 0;
}

