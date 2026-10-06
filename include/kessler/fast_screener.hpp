#pragma once
// The fast version of screen_bruteforce (screener.hpp): the same results, but
// it propagates objects on all CPU threads and finds pairs with the grid.

#include "kessler/screener.hpp"

#include <vector>

namespace kessler {

/// Same inputs, output and rules as screen_bruteforce.
std::vector<Hit> screen_grid(std::vector<Propagator>& objects, const ScreenConfig& config);

}  // namespace kessler
