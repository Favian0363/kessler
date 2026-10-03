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
    const auto result = kessler::parse_tle_stream_lenient(file);
    std::cout << "Parsed " << result.tles.size() << " objects";
    if (!result.errors.empty()) {
        std::cout << ", skipped " << result.errors.size() << " bad records:\n";
        constexpr std::size_t kMaxShown = 10;
        for (std::size_t i = 0; i < result.errors.size() && i < kMaxShown; ++i) {
            std::cout << "  " << result.errors[i] << '\n';
        }
        if (result.errors.size() > kMaxShown) {
            std::cout << "  ... and " << result.errors.size() - kMaxShown << " more\n";
        }
    } else {
        std::cout << '\n';
    }
    return 0;
}
