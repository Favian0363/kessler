#include "kessler/pairs.hpp"

#include <stdexcept>
#include <cmath>

namespace kessler {

void find_close_pairs_bruteforce(const std::vector<Vec3>& positions,
                                 const std::vector<char>& alive, double threshold_km, int step,
                                 std::vector<Hit>& out) {
    if ((positions.size() != alive.size()) || (threshold_km <= 0)){
        throw std::invalid_argument("find_close_pairs_bruteforce: invalid");
    }

    double sqr_threshold = threshold_km * threshold_km;

    for (std::size_t i=0; i < positions.size(); ++i) {
        if (!alive[i]){
            continue;
        }

        for (std::size_t j=i+1; j < positions.size(); ++j) {
            if (!alive[j]) {
                continue;
            }

            const double dx = positions[j].x - positions[i].x; 
            const double dy = positions[j].y - positions[i].y;
            const double dz = positions[j].z - positions[i].z;
            const double distsq = dx * dx + dy * dy + dz * dz;

            if (distsq < sqr_threshold) {
                out.push_back(Hit{
                        static_cast<std::uint32_t>(i),
                        static_cast<std::uint32_t>(j),
                        step,
                        std::sqrt(distsq)
                        });
            }
        }
    }
}

}  // namespace kessler
