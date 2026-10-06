#pragma once
// Turning encounters (runs of hits) into real close approaches, using SGP4.

#include "kessler/events.hpp"
#include "kessler/propagator.hpp"
#include "kessler/screener.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kessler {

struct Conjunction {
    std::uint32_t a{};        // index of the first object (a < b)
    std::uint32_t b{};        // index of the second object
    double tca_minutes{};     // time of closest approach, in minutes after config.start
    double miss_km{};         // distance between them at that moment
    double rel_speed_km_s{};  // how fast they pass each other at that moment
};

struct RefineStats {
    std::size_t refined = 0;  // runs searched for their closest moment
    std::size_t skipped = 0;  // runs that could not get within the miss distance
};

/// For each run: search between the sample before and the sample after its
/// closest sample for the exact moment the two objects are nearest, using
/// real SGP4 positions. Keep it if that distance is below `miss_km`.
///
///  - With `skip_impossible` (the default), runs that cannot get within
///    `miss_km` (see can_get_within) are skipped. The result is the same;
///    it is only faster.
///  - Runs are refined on all CPU threads. Each thread works on its own copy
///    of the two objects, so `objects` itself is never changed.
///  - Output is ordered by time of closest approach, then a, then b.
///
/// Throws std::invalid_argument if miss_km <= 0 or the config is bad.
std::vector<Conjunction> refine_runs(const std::vector<Propagator>& objects,
                                     const ScreenConfig& config, const std::vector<HitRun>& runs,
                                     double miss_km, bool skip_impossible = true,
                                     RefineStats* stats = nullptr);

}  // namespace kessler
