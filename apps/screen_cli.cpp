// kessler_screen: find close approaches in a TLE catalog.
//
// Usage: kessler_screen <catalog.tle> [hours=24] [step_seconds=10] [miss_km=5] [max_objects=0]
//                       [--brute | --compare]
//   max_objects = 0 means use every object in the file.
//   (default)   fast: spatial grid + all CPU threads
//   --brute     the slow brute-force answer key, one thread
//   --compare   run both, check they give identical hits, and report the speedup
//
// Thread count: set OMP_NUM_THREADS, e.g.  OMP_NUM_THREADS=4 ./kessler_screen ...
// The screening starts at the newest TLE epoch in the file, so every object
// is propagated forward from its own epoch.

#include "kessler/conjunctions.hpp"
#include "kessler/fast_screener.hpp"
#include "kessler/screener.hpp"
#include "kessler/sgp4_elements.hpp"
#include "kessler/tle.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace kessler;

namespace {

double seconds_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

int usage() {
    std::fprintf(stderr, "usage: kessler_screen <catalog.tle> [hours=24] [step_seconds=10] "
                         "[miss_km=5] [max_objects=0 (all)] [--brute | --compare]\n");
    return 1;
}

int thread_count() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

// Same pairs, steps and order, distances within 1e-9 km. Returns the number
// of the first differing hit, or -1 if identical.
long first_difference(const std::vector<Hit>& x, const std::vector<Hit>& y) {
    const std::size_t n = std::min(x.size(), y.size());
    for (std::size_t k = 0; k < n; ++k) {
        if (x[k].a != y[k].a || x[k].b != y[k].b || x[k].step != y[k].step ||
            std::fabs(x[k].distance_km - y[k].distance_km) > 1e-9) {
            return static_cast<long>(k);
        }
    }
    return x.size() == y.size() ? -1 : static_cast<long>(n);
}

}  // namespace

int main(int argc, char** argv) {
    // ---- Arguments: positional numbers plus optional --flags ----
    std::vector<std::string> positional;
    bool brute_only = false, compare = false;
    for (int k = 1; k < argc; ++k) {
        const std::string arg = argv[k];
        if (arg == "--brute") brute_only = true;
        else if (arg == "--compare") compare = true;
        else if (arg.rfind("--", 0) == 0) return usage();
        else positional.push_back(arg);
    }
    if (positional.empty() || (brute_only && compare)) return usage();

    double hours = 24.0, step_s = 10.0, miss_km = 5.0;
    std::size_t max_objects = 0;
    try {
        if (positional.size() > 1) hours = std::stod(positional[1]);
        if (positional.size() > 2) step_s = std::stod(positional[2]);
        if (positional.size() > 3) miss_km = std::stod(positional[3]);
        if (positional.size() > 4) max_objects = std::stoul(positional[4]);
    } catch (const std::exception&) {
        return usage();
    }

    // ---- Load the catalog ----
    std::ifstream file(positional[0]);
    if (!file) {
        std::fprintf(stderr, "cannot open %s\n", positional[0].c_str());
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

    // ---- Screening ----
    std::vector<Hit> hits;
    GridTimings breakdown;
    double t_brute = -1.0, t_grid = -1.0;
    long difference = -1;
    if (brute_only || compare) {
        std::vector<Propagator> copy = objects;  // its own copy, so runs can't affect each other
        const auto t0 = std::chrono::steady_clock::now();
        hits = screen_bruteforce(copy, cfg);
        t_brute = seconds_since(t0);
    }
    if (!brute_only) {
        std::vector<Propagator> copy = objects;
        const auto t0 = std::chrono::steady_clock::now();
        auto grid_hits = screen_grid(copy, cfg, &breakdown);
        t_grid = seconds_since(t0);
        if (compare) difference = first_difference(hits, grid_hits);
        hits = std::move(grid_hits);
    }

    auto t0 = std::chrono::steady_clock::now();
    const auto runs = group_into_runs(hits);
    const double t_group = seconds_since(t0);
    t0 = std::chrono::steady_clock::now();
    RefineStats refine_stats;
    auto conj = refine_runs(objects, cfg, runs, miss_km, true, &refine_stats);
    const double t_refine = seconds_since(t0);

    // In compare mode, also refine EVERY run (no skipping) and check the
    // answer is the same: proof that the skip rule never loses a real one.
    bool skip_rule_ok = true;
    double t_refine_all = -1.0;
    if (compare) {
        t0 = std::chrono::steady_clock::now();
        const auto all = refine_runs(objects, cfg, runs, miss_km, false);
        t_refine_all = seconds_since(t0);
        skip_rule_ok = all.size() == conj.size();
        for (std::size_t k = 0; skip_rule_ok && k < all.size(); ++k) {
            skip_rule_ok = all[k].a == conj[k].a && all[k].b == conj[k].b &&
                           std::fabs(all[k].miss_km - conj[k].miss_km) <= 1e-9;
        }
    }

    // ---- Summary ----
    std::printf("objects        %zu  (skipped %zu bad records, %zu SGP4 init failures)\n",
                objects.size(), parsed.errors.size(), init_failures);
    std::printf("window         %s  ->  %s UTC\n", to_utc_string(start).c_str(),
                to_utc_string(add_minutes(start, cfg.duration_minutes)).c_str());
    std::printf("sampling       every %.1f s, %d samples\n", step_s, sample_count(cfg));
    std::printf("thresholds     candidate %.1f km, miss %.2f km\n", cfg.threshold_km, miss_km);
    if (t_brute >= 0.0) std::printf("brute force    %.2f s  (1 thread)\n", t_brute);
    if (t_grid >= 0.0) {
        std::printf("grid           %.2f s  (%d threads)\n", t_grid, thread_count());
        std::printf("  propagation  %.2f s  (all threads)\n", breakdown.propagation_s);
        std::printf("  grid setup   %.2f s  (one thread)\n", breakdown.setup_s);
        std::printf("  pair search  %.2f s  (all threads)\n", breakdown.search_s);
        std::printf("  merge        %.2f s  (one thread)\n", breakdown.merge_s);
    }
    if (compare) {
        std::printf("speedup        %.1fx\n", t_brute / t_grid);
        if (difference < 0) {
            std::printf("identical      YES, all %zu hits match\n", hits.size());
        } else {
            std::printf("identical      NO, first difference at hit #%ld\n", difference);
        }
    }
    std::printf("hits / runs    %zu / %zu  (grouping %.3f s)\n", hits.size(), runs.size(), t_group);
    std::printf("refinement     %.2f s  (refined %zu runs, skipped %zu that can't get within %.2f km)\n",
                t_refine, refine_stats.refined, refine_stats.skipped, miss_km);
    if (compare) {
        std::printf("skip rule      %s  (refining every run took %.2f s)\n",
                    skip_rule_ok ? "OK, same conjunctions with and without skipping"
                                 : "FAILED, skipping changed the answer",
                    t_refine_all);
    }
    std::printf("conjunctions   %zu\n\n", conj.size());

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
    return (compare && (difference >= 0 || !skip_rule_ok)) ? 2 : 0;
}
