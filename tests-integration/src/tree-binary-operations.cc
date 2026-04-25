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
        std::cerr << "err \n";
        return EXIT_FAILURE;
    }
    if (!file_2.is_open()) {
        std::cerr << "err \n";
        return EXIT_FAILURE;
    }

    Nfta lhs = parse_from_mata(file_1, &alphabet);
    Nfta rhs = parse_from_mata(file_2, &alphabet);

    file_1.close();
    file_2.close();

    // Setting precision of the times to fixed points and 4 decimal places
    std::cout << std::fixed << std::setprecision(4);

    Nfta intersect_aut;
    TIME_BEGIN(intersection);
    intersect_aut = intersection(lhs, rhs);
    TIME_END(intersection);

    return EXIT_SUCCESS;
}
