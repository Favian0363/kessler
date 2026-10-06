// Tests for refining encounters into close approaches. Uses real SGP4.

#include "kessler/conjunctions.hpp"
#include "kessler/screener.hpp"
#include "kessler/sgp4_elements.hpp"
#include "kessler/tle.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <string>
#include <vector>

using Catch::Matchers::WithinAbs;
using namespace kessler;

namespace {

const std::string kIss1 = "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927";
const std::string kIss2 = "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537";
// The same ISS elements with an epoch 0.0001 days (8.64 s) later. SGP4 then puts
// it exactly where the ISS was 8.64 s earlier: about 66 km behind, at all times.
const std::string kTrail1 = "1 25544U 98067A   08264.51792528 -.00002182  00000-0 -11606-4 0  2928";
// The ISS orbit, but 0.1 rev/day faster (so about 28 km lower) and starting
// 0.5 degrees behind. It catches up with the ISS and passes underneath it
// about 20 minutes in: a real pass, with the distance shrinking and then growing.
const std::string kFast2 = "2 25544  51.6416 247.4627 0006703 130.5360 324.5288 15.82125391563532";
const std::string kGeo1 = "1 25545U 98067B   08264.26782528  .00000000  00000-0  00000-0 0  9991";
const std::string kGeo2 = "2 25545   0.0500 100.0000 0000100   0.0000  90.0000  1.00270000   100";

const JulianDate kStart = tle_epoch_jd(2008, 264.51782528);

Propagator make_propagator(const std::string& l1, const std::string& l2) {
    return Propagator(to_sgp4_elements(parse_tle(l1, l2)));
}

// The whole pipeline: screen with the candidate threshold, group, refine.
std::vector<Conjunction> find_all(std::vector<Propagator>& objs, double minutes, double step_s,
                                  double miss_km, std::vector<HitRun>* runs_out = nullptr) {
    const ScreenConfig cfg{kStart, minutes, step_s, candidate_threshold_km(miss_km, step_s)};
    const auto runs = group_into_runs(screen_bruteforce(objs, cfg));
    if (runs_out != nullptr) *runs_out = runs;
    return refine_runs(objs, cfg, runs, miss_km);
}

double distance_km(const StateVector& a, const StateVector& b) {
    const double dx = a.r_km[0] - b.r_km[0];
    const double dy = a.r_km[1] - b.r_km[1];
    const double dz = a.r_km[2] - b.r_km[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

TEST_CASE("two copies of one object: one conjunction at 0 km and 0 km/s", "[conjunctions]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kIss1, kIss2));
    const auto c = find_all(objs, 60.0, 10.0, 5.0);
    REQUIRE(c.size() == std::size_t{1});
    CHECK_THAT(c[0].miss_km, WithinAbs(0.0, 1e-9));
    CHECK_THAT(c[0].rel_speed_km_s, WithinAbs(0.0, 1e-9));
}

TEST_CASE("the ISS and a geostationary object never conjoin", "[conjunctions]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kGeo1, kGeo2));
    CHECK(find_all(objs, 1440.0, 60.0, 5.0).empty());
}

TEST_CASE("an object trailing 66 km behind gives hits but no 5 km conjunction", "[conjunctions]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kTrail1, kIss2));
    std::vector<HitRun> runs;
    const auto c = find_all(objs, 120.0, 10.0, 5.0, &runs);
    CHECK(runs.size() == std::size_t{1});  // always inside the 118 km net: one long run
    CHECK(c.empty());                      // but never within 5 km
}

TEST_CASE("the reported miss distance is what SGP4 gives at the reported time", "[conjunctions]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kTrail1, kIss2));
    std::vector<HitRun> runs;
    // A huge miss distance so the trailing pair counts as a conjunction.
    const auto c = find_all(objs, 120.0, 10.0, 1000.0, &runs);
    REQUIRE(c.size() == std::size_t{1});
    REQUIRE(runs.size() == std::size_t{1});

    CHECK(c[0].tca_minutes >= 0.0);
    CHECK(c[0].tca_minutes <= 120.0);
    CHECK(c[0].miss_km <= runs[0].closest_km + 1e-9);  // refining never does worse than the samples

    Propagator a = make_propagator(kIss1, kIss2);
    Propagator b = make_propagator(kTrail1, kIss2);
    StateVector sa, sb;
    const JulianDate t = add_minutes(kStart, c[0].tca_minutes);
    REQUIRE(a.propagate_to(t, sa));
    REQUIRE(b.propagate_to(t, sb));
    CHECK_THAT(c[0].miss_km, WithinAbs(distance_km(sa, sb), 1e-6));
}

TEST_CASE("a real pass: the closest moment is found between samples, near the closest sample",
          "[conjunctions]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kIss1, kFast2));
    std::vector<HitRun> runs;
    const double step_s = 10.0;
    const auto c = find_all(objs, 120.0, step_s, 50.0, &runs);

    REQUIRE(runs.size() == std::size_t{1});
    REQUIRE(c.size() == std::size_t{1});
    // The pass happens in the middle of the run, not at its start.
    CHECK(runs[0].closest_step > runs[0].first_step);
    // Refining searches around the closest sample: the answer is within one step of it...
    const double closest_sample_min = runs[0].closest_step * step_s / 60.0;
    CHECK(std::fabs(c[0].tca_minutes - closest_sample_min) <= step_s / 60.0);
    // ...and is at least as close as the best sample.
    CHECK(c[0].miss_km <= runs[0].closest_km + 1e-9);
}
