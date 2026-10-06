// kessler_screen: find close approaches in a TLE catalog (brute force for now).
//
// Usage: kessler_screen <catalog.tle> [hours=24] [step_seconds=10] [miss_km=5] [max_objects=0]
//   max_objects = 0 means use every object in the file.
//
// The screening starts at the newest TLE epoch in the file, so every object
// is propagated forward from its own epoch.

#include "kessler/conjunctions.hpp"
#include "kessler/screener.hpp"
#include "kessler/sgp4_elements.hpp"
#include "kessler/tle.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace kessler;

namespace {

double seconds_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

int usage() {
    std::fprintf(stderr, "usage: kessler_screen <catalog.tle> [hours=24] [step_seconds=10] "
                         "[miss_km=5] [max_objects=0 (all)]\n");
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) return usage();
    double hours = 24.0, step_s = 10.0, miss_km = 5.0;
    std::size_t max_objects = 0;
    try {
        if (argc > 2) hours = std::stod(argv[2]);
        if (argc > 3) step_s = std::stod(argv[3]);
        if (argc > 4) miss_km = std::stod(argv[4]);
        if (argc > 5) max_objects = std::stoul(argv[5]);
    } catch (const std::exception&) {
        return usage();
    }

    // ---- Load the catalog ----
    std::ifstream file(argv[1]);
    if (!file) {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }
    auto parsed = parse_tle_stream_lenient(file);
    if (max_objects > 0 && parsed.tles.size() > max_objects) parsed.tles.resize(max_objects);

    std::vector<Propagator> objects;
    std::vector<std::string> names;
    std::size_t init_failures = 0;
    for (const Tle& t : parsed.tles) {
        try {
            objects.emplace_back(to_sgp4_elements(t));
            names.push_back(t.name.empty() ? "-" : t.name.substr(0, 24));
        } catch (const Sgp4InitError&) {
            ++init_failures;
        }
    }
    if (objects.size() < 2) {
        std::fprintf(stderr, "need at least 2 usable objects\n");
        return 1;
    }

    JulianDate start = objects.front().epoch();
    for (const auto& o : objects) {
        if (o.epoch().total() > start.total()) start = o.epoch();
    }

    const ScreenConfig cfg{start, hours * 60.0, step_s, candidate_threshold_km(miss_km, step_s)};
    const int samples = sample_count(cfg);
    const double n = static_cast<double>(objects.size());
    const double pair_checks = n * (n - 1.0) / 2.0 * samples;

    // ---- Run the three stages, timing each ----
    auto t0 = std::chrono::steady_clock::now();
    const auto hits = screen_bruteforce(objects, cfg);
    const double t_screen = seconds_since(t0);

    t0 = std::chrono::steady_clock::now();
    const auto runs = group_into_runs(hits);
    const double t_group = seconds_since(t0);

    t0 = std::chrono::steady_clock::now();
    auto conj = refine_runs(objects, cfg, runs, miss_km);
    const double t_refine = seconds_since(t0);

    // ---- Summary ----
    std::printf("objects        %zu  (skipped %zu bad records, %zu SGP4 init failures)\n",
                objects.size(), parsed.errors.size(), init_failures);
    std::printf("window         %s  ->  %s UTC\n", to_utc_string(start).c_str(),
                to_utc_string(add_minutes(start, cfg.duration_minutes)).c_str());
    std::printf("sampling       every %.1f s, %d samples\n", step_s, samples);
    std::printf("thresholds     candidate %.1f km, miss %.2f km\n", cfg.threshold_km, miss_km);
    std::printf("pair checks    %.3e in %.2f s  (%.3e checks/s, includes SGP4)\n", pair_checks,
                t_screen, pair_checks / t_screen);
    std::printf("hits / runs    %zu / %zu  (grouping %.3f s)\n", hits.size(), runs.size(), t_group);
    std::printf("conjunctions   %zu  (refinement %.3f s)\n", conj.size(), t_refine);
    std::printf("total          %.2f s\n\n", t_screen + t_group + t_refine);

    // ---- Closest approaches first ----
    std::sort(conj.begin(), conj.end(),
              [](const Conjunction& x, const Conjunction& y) { return x.miss_km < y.miss_km; });
    const std::size_t shown = std::min<std::size_t>(conj.size(), 20);
    if (shown > 0) {
        std::printf("%-23s  %-24s  %-24s  %9s  %10s\n", "TCA (UTC)", "object A", "object B",
                    "miss km", "rel km/s");
    }
    for (std::size_t k = 0; k < shown; ++k) {
        const Conjunction& c = conj[k];
        const std::string a = std::to_string(objects[c.a].catalog_number()) + " " + names[c.a];
        const std::string b = std::to_string(objects[c.b].catalog_number()) + " " + names[c.b];
        std::printf("%-23s  %-24.24s  %-24.24s  %9.3f  %10.3f\n",
                    to_utc_string(add_minutes(start, c.tca_minutes)).c_str(), a.c_str(), b.c_str(),
                    c.miss_km, c.rel_speed_km_s);
    }
    if (conj.size() > shown) std::printf("... and %zu more\n", conj.size() - shown);
    return 0;
}
