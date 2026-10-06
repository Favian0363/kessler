#pragma once
// Turning encounters (runs of hits) into real close approaches, using SGP4.

#include "kessler/events.hpp"
#include "kessler/propagator.hpp"
#include "kessler/screener.hpp"

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

/// For each run: search between the sample before and the sample after its
/// closest sample for the exact moment the two objects are nearest, using
/// real SGP4 positions. Keep it if that distance is below `miss_km`.
/// Output is ordered by time of closest approach.
/// Throws std::invalid_argument if miss_km <= 0 or the config is bad.
std::vector<Conjunction> refine_runs(std::vector<Propagator>& objects, const ScreenConfig& config,
                                     const std::vector<HitRun>& runs, double miss_km);

}  // namespace kessler
