#include "kessler/screener.hpp"

#include <cmath>
#include <stdexcept>

namespace kessler {

// Written for you as an example of validating input at the edge of a function.
int sample_count(const ScreenConfig& c) {
    if (!(c.step_seconds > 0.0)) throw std::invalid_argument("step_seconds must be > 0");
    if (!(c.duration_minutes >= 0.0)) throw std::invalid_argument("duration_minutes must be >= 0");
    if (!(c.threshold_km > 0.0)) throw std::invalid_argument("threshold_km must be > 0");
    // The tiny 1e-9 stops a result like 359.99999999 (from floating-point
    // rounding) from being cut down to 359 by floor().
    return static_cast<int>(std::floor(c.duration_minutes * 60.0 / c.step_seconds + 1e-9)) + 1;
}

std::vector<Hit> screen_bruteforce(std::vector<Propagator>& objects, const ScreenConfig& config) {
    // TODO(you), in this order:
    //   1. int steps = sample_count(config);   (this also validates the config)
    //   2. For each object i, work out ONCE how many minutes there are from that
    //      object's own epoch to config.start:
    //          offset[i] = minutes_between(objects[i].epoch(), config.start)
    //      Every object has its own epoch, so this number is different for each.
    //   3. Make `alive` (all 1) and `positions` (one Vec3 per object), and an
    //      empty `hits` vector.
    //   4. For k = 0 .. steps-1:
    //        - the time since the start, in minutes, is k * step_seconds / 60
    //        - for each object that is still alive:
    //              objects[i].propagate(offset[i] + that_time, state)
    //              if it returns false: set alive[i] = 0 and move on
    //              otherwise copy state.r_km[0..2] into positions[i]
    //        - call find_close_pairs_bruteforce(positions, alive,
    //                                           config.threshold_km, k, hits)
    //   5. return hits
    (void)objects;
    (void)config;
    throw std::logic_error("screen_bruteforce: not implemented");
}

}  // namespace kessler
