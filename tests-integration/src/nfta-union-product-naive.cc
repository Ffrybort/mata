/**
* NOTE: Input automata, that are of type `NFA-bits` are mintermized!
 *  - If you want to skip mintermization, set the variable `MINTERMIZE_AUTOMATA` below to `false`
 */

#include "utils/utils.hh"
#include "mata/nfta/nfta.hh"
#include "mata/nfta/builder.hh"

using namespace mata::nfta;



int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Input files missing\n";
        return EXIT_FAILURE;
    }

    mata::OnTheFlyAlphabet alphabet;
    std::ifstream file_1(argv[1]);
    std::ifstream file_2(argv[2]);
    if (!file_1.is_open()) {
        std::cout << "could not open file \n";
        return EXIT_FAILURE;
    }
    if (!file_2.is_open()) {
        std::cout << "could not open file \n";
        return EXIT_FAILURE;
    }

    Nfta lhs = parse_from_mata(file_1, &alphabet);
    Nfta rhs = parse_from_mata(file_2, &alphabet);

    file_1.close();
    file_2.close();

    // Setting precision of the times to fixed points and 4 decimal places
    std::cout << std::fixed << std::setprecision(4);
    Nfta res;
    TIME_BEGIN(union_product);
    auto symbols = lhs.delta.get_used_symbols_arities();
    symbols.insert(rhs.delta.get_used_symbols_arities());
    lhs.make_complete(&symbols);
    rhs.make_complete(&symbols);
    res = union_det_on_complete_naive(lhs, rhs);
    TIME_END(union_product);

    return EXIT_SUCCESS;
}
