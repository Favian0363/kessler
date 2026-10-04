#include "kessler/pairs.hpp"

#include <stdexcept>

namespace kessler {

void find_close_pairs_bruteforce(const std::vector<Vec3>& positions,
                                 const std::vector<char>& alive, double threshold_km, int step,
                                 std::vector<Hit>& out) {
    // TODO(you), in this order:
    //   1. Validate: positions.size() == alive.size() and threshold_km > 0.
    //      Throw std::invalid_argument otherwise.
    //   2. Compare SQUARED distances with threshold_km * threshold_km.
    //      sqrt is slow, and "is it closer than X?" doesn't need it.
    //   3. Loop over every pair i < j. Skip the pair if either object is not alive.
    //   4. If the pair is STRICTLY closer than the threshold, take the real
    //      distance with std::sqrt (include <cmath>) and push_back a Hit.
    //   Do NOT clear `out`: it already holds hits from earlier time steps.
    // Hint: i and j are std::size_t but Hit::a / Hit::b are std::uint32_t.
    //       The compiler warns about that conversion; use static_cast.
    
    throw std::logic_error("find_close_pairs_bruteforce: not implemented");
}

}  // namespace kessler
