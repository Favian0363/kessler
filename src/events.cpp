#include "kessler/events.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace kessler {

std::vector<HitRun> group_into_runs(std::vector<Hit> hits) {
    // Put each pair's hits next to each other, in time order.
    std::sort(hits.begin(), hits.end(), [](const Hit& x, const Hit& y) {
        if (x.a != y.a) return x.a < y.a;
        if (x.b != y.b) return x.b < y.b;
        return x.step < y.step;
    });

    std::vector<HitRun> runs;
    for (const Hit& h : hits) {
        const bool continues_last_run = !runs.empty() && runs.back().a == h.a &&
                                        runs.back().b == h.b && runs.back().last_step + 1 == h.step;
        if (continues_last_run) {
            HitRun& r = runs.back();
            r.last_step = h.step;
            if (h.distance_km < r.closest_km) {
                r.closest_km = h.distance_km;
                r.closest_step = h.step;
            }
        } else {
            runs.push_back({h.a, h.b, h.step, h.step, h.step, h.distance_km});
        }
    }
    return runs;
}

double candidate_threshold_km(double miss_km, double step_seconds) {
    if (!(miss_km > 0.0)) throw std::invalid_argument("miss_km must be > 0");
    if (!(step_seconds > 0.0)) throw std::invalid_argument("step_seconds must be > 0");
    constexpr double kMaxRelativeSpeedKmS = 22.4;  // 2 x Earth escape speed
    constexpr double kCurvatureMarginKm = 1.0;
    return miss_km + kMaxRelativeSpeedKmS * step_seconds / 2.0 + kCurvatureMarginKm;
}

double golden_section_minimize(const std::function<double(double)>& f, double lo, double hi,
                               double tol) {
    if (!(hi >= lo)) throw std::invalid_argument("golden_section_minimize: need lo <= hi");
    if (!(tol > 0.0)) throw std::invalid_argument("golden_section_minimize: need tol > 0");

    const double r = (std::sqrt(5.0) - 1.0) / 2.0;  // about 0.618
    double a = lo, b = hi;
    double c = b - r * (b - a);  // left probe
    double d = a + r * (b - a);  // right probe
    double fc = f(c), fd = f(d);

    // 200 rounds is far more than needed (each keeps 61.8%); it is only a safety stop.
    for (int round = 0; round < 200 && (b - a) > tol; ++round) {
        if (fc < fd) {  // the valley is in [a, d]: drop the right part
            b = d;
            d = c;
            fd = fc;
            c = b - r * (b - a);
            fc = f(c);
        } else {        // the valley is in [c, b]: drop the left part
            a = c;
            c = d;
            fc = fd;
            d = a + r * (b - a);
            fd = f(d);
        }
    }
    return 0.5 * (a + b);
}

}  // namespace kessler
