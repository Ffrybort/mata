#include "mata/nfta/builder.hh"
#include "mata/nfta/nfta.hh"
#include "utils/utils.hh"

using namespace mata::nfta;


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Input files missing\n";
        return EXIT_FAILURE;
    }

    mata::nfta::RankedOnTheFlyAlphabet alphabet;
    std::ifstream file_1(argv[1]);
    if (!file_1.is_open()) {
        std::cerr << "err \n";
        return EXIT_FAILURE;
    }

    Nfta aut = parse_from_mata(file_1, &alphabet);

    file_1.close();

    // Setting precision of the times to fixed points and 4 decimal places
    std::cout << std::fixed << std::setprecision(4);

    Nfta res;
    TIME_BEGIN(complement_td);
    res = complement_top_down(aut);
    TIME_END(complement_td);

    return EXIT_SUCCESS;
}
