#include "kessler/tle.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: kessler <catalog.tle>\n";
        return 1;
    }
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "error: cannot open " << argv[1] << '\n';
        return 1;
    }
    try {
        const auto catalog = kessler::parse_tle_stream(file);
        std::cout << "Parsed " << catalog.size() << " objects\n";
    } catch (const kessler::TleParseError& e) {
        std::cerr << "parse error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
