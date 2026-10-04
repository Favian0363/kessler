#pragma once
// Runs the brute-force screening over a span of time.
//
// For each time sample: move every object to that time with SGP4, then hand
// the positions to find_close_pairs_bruteforce() (pairs.hpp).

#include "kessler/pairs.hpp"
#include "kessler/propagator.hpp"
#include "kessler/time.hpp"

#include <vector>

namespace kessler {

struct ScreenConfig {
    JulianDate start{};         // time of the first sample (UTC)
    double duration_minutes{};  // how long to screen for
    double step_seconds{};      // time between samples
    double threshold_km{};      // report pairs closer than this
};

/// How many time samples a run takes: floor(duration / step) + 1.
/// The +1 is because the first sample is at time 0 and the last is at the
/// end, so 60 minutes at 10 s steps is samples at 0, 10, ... 3600 = 361.
/// Throws std::invalid_argument for a bad config (step <= 0, duration < 0,
/// threshold <= 0). Already written for you, as an example.
int sample_count(const ScreenConfig& config);

/// Screens every pair of `objects` at every time sample.
///
///  - Hits come out in order of step, then a, then b.
///  - `a` and `b` are positions in the `objects` vector, not catalog numbers.
///  - An object that fails to propagate (decayed, bad elements) is dropped for
///    the rest of the run and never used again.
///  - Throws std::invalid_argument for a bad config.
///
/// `objects` is not const because propagating changes SGP4's internal state.
std::vector<Hit> screen_bruteforce(std::vector<Propagator>& objects, const ScreenConfig& config);

}  // namespace kessler
