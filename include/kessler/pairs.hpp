#pragma once
// Finding pairs of objects that are close to each other at one moment in time.
//
// This is the slow, obviously-correct version ("brute force"): it checks every
// pair. Later, faster versions (a spatial grid) must return exactly the same
//
// Nothing in this file knows about satellites or SGP4. It only sees points in
// space. That is on purpose: it means we can test it with made-up points whose
// distances we can work out by hand.

#include <cstdint>
#include <vector>

namespace kessler {

struct Vec3 {
    double x{}, y{}, z{};  // kilometers
};

// One pair of objects that were closer than the threshold at one time sample.
struct Hit {
    std::uint32_t a{};     // index of the first object (always a < b)
    std::uint32_t b{};     // index of the second object
    int step{};            // which time sample this was (0 = the start time)
    double distance_km{};  // how far apart they were at that sample
};

/// Checks every pair (i, j) with i < j and appends a Hit for each pair that is
/// strictly closer than `threshold_km`.
///
///  - `positions[i]` is object i's position in km.
///  - `alive[i]` is 1 if object i has a valid position right now, 0 if not
///    (for example, it decayed). Pairs involving a 0 are skipped.
///  - `step` is just copied into each Hit.
///  - `out` is APPENDED to, never cleared, because the caller calls this once
///    per time step and keeps adding to the same list.
///  - Hits come out in order of i, then j.
///
/// Throws std::invalid_argument if `positions` and `alive` have different
/// sizes, or if `threshold_km` is not greater than zero.
void find_close_pairs_bruteforce(const std::vector<Vec3>& positions,
                                 const std::vector<char>& alive, double threshold_km, int step,
                                 std::vector<Hit>& out);

}  // namespace kessler
