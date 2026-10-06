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

// Where the time goes inside the fast screener, added up over all steps.
// "one thread" parts limit how much more threads can help (Amdahl's law).
struct GridTimings {
    double propagation_s = 0.0;  // SGP4 for every object (all threads)
    double setup_s = 0.0;        // working out cubes and sorting (one thread)
    double search_s = 0.0;       // checking neighbors (all threads)
    double merge_s = 0.0;        // merging and sorting the hits (one thread)
};

/// Same inputs, output and errors as find_close_pairs_bruteforce. Also throws
/// std::out_of_range if a position is too far from the origin for the grid
/// (about a million cells away). Uses all CPU threads (OpenMP); the result
/// is the same no matter how many threads run.
/// If `timings` is given, the time spent in each part is added to it.
void find_close_pairs_grid(const std::vector<Vec3>& positions, const std::vector<char>& alive,
                           double threshold_km, int step, std::vector<Hit>& out,
                           GridTimings* timings = nullptr);

}  // namespace kessler
