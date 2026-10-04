// Tests for the time-stepping driver. These use real SGP4 (through Propagator),
// so they are built only when Vallado's files are present.

#include "kessler/screener.hpp"
#include "kessler/sgp4_elements.hpp"
#include "kessler/tle.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using Catch::Matchers::WithinAbs;
using namespace kessler;

namespace {

// The ISS, from the TLE format documentation.
const std::string kIss1 = "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927";
const std::string kIss2 = "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537";

// A made-up geostationary-style object: about 42,000 km from Earth's center
// (the ISS is about 6,700 km), a different catalog number, and an epoch 6
// hours EARLIER than the ISS TLE. Checksums computed and checked.
const std::string kGeo1 = "1 25545U 98067B   08264.26782528  .00000000  00000-0  00000-0 0  9991";
const std::string kGeo2 = "2 25545   0.0500 100.0000 0000100   0.0000  90.0000  1.00270000   100";

// Vallado's test record 33333: a decaying orbit. Its checksum digits in his
// file are wrong, so they are corrected here (2 and 0).
const std::string kDecay1 = "1 33333U 05037B   05333.02012661  .25992681  00000-0  24476-3 0  1532";
const std::string kDecay2 = "2 33333  96.4736 157.9986 9950000 244.0492 110.6523  4.00004038 10700";

Propagator make_propagator(const std::string& l1, const std::string& l2) {
    return Propagator(to_sgp4_elements(parse_tle(l1, l2)));
}

double distance(const StateVector& a, const StateVector& b) {
    const double dx = a.r_km[0] - b.r_km[0];
    const double dy = a.r_km[1] - b.r_km[1];
    const double dz = a.r_km[2] - b.r_km[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

const JulianDate kIssEpoch = tle_epoch_jd(2008, 264.51782528);

}  // namespace

TEST_CASE("sample count includes both the first and the last sample", "[screener]") {
    CHECK(sample_count({kIssEpoch, 60.0, 10.0, 5.0}) == 361);  // 0, 10, ..., 3600 s
    CHECK(sample_count({kIssEpoch, 0.0, 10.0, 5.0}) == 1);     // just the start time
    CHECK(sample_count({kIssEpoch, 1.0, 25.0, 5.0}) == 3);     // 0, 25, 50 s (75 > 60)
}

TEST_CASE("a bad config is rejected", "[screener]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    CHECK_THROWS_AS(screen_bruteforce(objs, {kIssEpoch, 60.0, 0.0, 5.0}), std::invalid_argument);
    CHECK_THROWS_AS(screen_bruteforce(objs, {kIssEpoch, -1.0, 10.0, 5.0}), std::invalid_argument);
    CHECK_THROWS_AS(screen_bruteforce(objs, {kIssEpoch, 60.0, 10.0, 0.0}), std::invalid_argument);
}

TEST_CASE("zero or one object gives no hits", "[screener]") {
    std::vector<Propagator> none;
    CHECK(screen_bruteforce(none, {kIssEpoch, 60.0, 10.0, 5.0}).empty());
    std::vector<Propagator> one;
    one.push_back(make_propagator(kIss1, kIss2));
    CHECK(screen_bruteforce(one, {kIssEpoch, 60.0, 10.0, 5.0}).empty());
}

TEST_CASE("two copies of the same object are always 0 km apart", "[screener]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kIss1, kIss2));

    const auto hits = screen_bruteforce(objs, {kIssEpoch, 60.0, 10.0, 80.0});

    REQUIRE(hits.size() == std::size_t{361});  // one pair, at every sample
    for (std::size_t k = 0; k < hits.size(); ++k) {
        CHECK(hits[k].a == 0u);
        CHECK(hits[k].b == 1u);
        CHECK(hits[k].step == static_cast<int>(k));
        CHECK_THAT(hits[k].distance_km, WithinAbs(0.0, 1e-9));
    }
}

TEST_CASE("the ISS and a geostationary object are never within 1000 km", "[screener]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kIss1, kIss2));
    objs.push_back(make_propagator(kGeo1, kGeo2));
    CHECK(screen_bruteforce(objs, {kIssEpoch, 1440.0, 60.0, 1000.0}).empty());
}

TEST_CASE("hit distances match a direct computation (objects with different epochs)", "[screener]") {
    Propagator iss = make_propagator(kIss1, kIss2);
    Propagator geo = make_propagator(kGeo1, kGeo2);
    std::vector<Propagator> objs;
    objs.push_back(iss);
    objs.push_back(geo);

    // A threshold so large that every sample is a hit.
    const ScreenConfig cfg{kIssEpoch, 120.0, 60.0, 1.0e6};
    const auto hits = screen_bruteforce(objs, cfg);
    REQUIRE(hits.size() == std::size_t{121});

    for (const Hit& h : hits) {
        // Move both objects to the same moment with propagate_to(), which
        // handles each object's own epoch, and measure the distance directly.
        const JulianDate t{kIssEpoch.day, kIssEpoch.frac + h.step * cfg.step_seconds / 86400.0};
        StateVector a, b;
        REQUIRE(iss.propagate_to(t, a));
        REQUIRE(geo.propagate_to(t, b));
        CHECK_THAT(h.distance_km, WithinAbs(distance(a, b), 1e-6));  // within 1 mm
    }
}

// Needs real SGP4: in the Vallado comparison, this record stops producing
// positions after 5 samples at 5-minute steps (it is decaying).
TEST_CASE("an object that stops propagating is dropped for the rest of the run",
          "[screener][real-sgp4]") {
    std::vector<Propagator> objs;
    objs.push_back(make_propagator(kDecay1, kDecay2));
    objs.push_back(make_propagator(kDecay1, kDecay2));

    const ScreenConfig cfg{tle_epoch_jd(2005, 333.02012661), 150.0, 300.0, 1.0};
    REQUIRE(sample_count(cfg) == 31);

    const auto hits = screen_bruteforce(objs, cfg);
    REQUIRE(hits.size() == std::size_t{5});  // steps 0..4, then both are gone
    for (std::size_t k = 0; k < hits.size(); ++k) {
        CHECK(hits[k].step == static_cast<int>(k));
    }
}
