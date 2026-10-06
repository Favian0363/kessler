#pragma once
// The fast version of find_close_pairs_bruteforce (pairs.hpp).
//
// Idea: chop space into cubes ("cells") a little wider than the threshold.
// Two objects closer than the threshold must be in the same cell or in
// touching cells, so each object only needs to be compared with the objects
// in its own cell and the 26 cells around it, not with every other object.
//
// It must return EXACTLY what find_close_pairs_bruteforce returns, in the same
// order. The tests compare the two on thousands of random points.

#include "kessler/pairs.hpp"

#include <vector>

namespace kessler {

/// Same inputs, output and errors as find_close_pairs_bruteforce. Also throws
/// std::out_of_range if a position is too far from the origin for the grid
/// (about a million cells away). Uses all CPU threads (OpenMP); the result
/// is the same no matter how many threads run.
void find_close_pairs_grid(const std::vector<Vec3>& positions, const std::vector<char>& alive,
                           double threshold_km, int step, std::vector<Hit>& out);

}  // namespace kessler
