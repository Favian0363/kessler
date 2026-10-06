#pragma once
// Shared by the grid tests: compares two hit lists.

#include "kessler/pairs.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace kessler {

/// Index of the first hit that differs, or -1 if the lists match: same pairs,
/// same steps, same order, and distances within 1e-9 km (1 micrometer).
/// The distance tolerance allows for the brute force adding up dx^2, dy^2,
/// dz^2 in a different order, which can change the last digit.
inline long first_difference(const std::vector<Hit>& x, const std::vector<Hit>& y) {
    const std::size_t n = std::min(x.size(), y.size());
    for (std::size_t k = 0; k < n; ++k) {
        if (x[k].a != y[k].a || x[k].b != y[k].b || x[k].step != y[k].step ||
            std::fabs(x[k].distance_km - y[k].distance_km) > 1e-9) {
            return static_cast<long>(k);
        }
    }
    return x.size() == y.size() ? -1 : static_cast<long>(n);
}

}  // namespace kessler
