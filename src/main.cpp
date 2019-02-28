#include "psass.h"
#include "pileup_converter.h"


int main(int argc, char *argv[]) {

    if (std::string(argv[1]) == "analyze") {

        std::cout << "PSASS started." << std::endl;

        Psass psass(argc, argv);
        psass.run();

        std::cout << "\nPSASS ended successfully." << std::endl;

    } else if (std::string(argv[1]) == "convert") {

        PileupConverter pileup_converter(argc, argv);
        pileup_converter.run();

    } else {

        std::cout << std::endl << "Usage: poolsex <command> [options]" << std::endl << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "        analyze    Compute metrics from either the resulting file from \"convert\" or a sync file from popoolation2" << std::endl;
        std::cout << "        convert    Convert a pileup file to a synchronized pool file. Accepts input from stdin with \"-\"" << std::endl;
        std::cout << std::endl;
    }


    return 0;
}
