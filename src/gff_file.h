#pragma once
#include "utils.h"

struct Gene {
    std::string contig;
    std::string start;
    std::string end;
    std::string name;
    std::string product;
    uint coding_length;
    uint noncoding_length;
    uint coverage[4]; // Coding male, Non-coding male, Coding female, Non-coding female
    uint snps[4]; // Coding male, Non-coding male, Coding female, Non-coding female
};

void read_gff_file(std::ifstream& input_file, std::unordered_map<std::string, std::unordered_map<uint, std::pair<std::string, bool>>>& regions, std::unordered_map<std::string, Gene>& genes);
