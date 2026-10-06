// Tests for grouping hits into encounters and the two math helpers. No SGP4.

#include "kessler/events.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <stdexcept>
#include <vector>

using Catch::Matchers::WithinAbs;
using namespace kessler;

TEST_CASE("hits are grouped into runs of consecutive steps, per pair", "[events]") {
    // Given in the screener's order (by step), not grouped by pair.
    const std::vector<Hit> hits{
        {0, 1, 3, 50.0}, {0, 1, 4, 20.0}, {0, 2, 4, 10.0}, {0, 1, 5, 40.0},
        {0, 1, 9, 70.0}, {0, 1, 10, 60.0},
    };
    const auto runs = group_into_runs(hits);

    REQUIRE(runs.size() == std::size_t{3});
    // Pair (0,1), steps 3-5, closest at step 4 (20 km).
    CHECK(runs[0].a == 0u);
    CHECK(runs[0].b == 1u);
    CHECK(runs[0].first_step == 3);
    CHECK(runs[0].last_step == 5);
    CHECK(runs[0].closest_step == 4);
    CHECK_THAT(runs[0].closest_km, WithinAbs(20.0, 0.0));
    // Pair (0,1) again, steps 9-10: the gap after step 5 starts a new encounter.
    CHECK(runs[1].first_step == 9);
    CHECK(runs[1].last_step == 10);
    CHECK(runs[1].closest_step == 10);
    // Pair (0,2), a one-step run.
    CHECK(runs[2].b == 2u);
    CHECK(runs[2].first_step == 4);
    CHECK(runs[2].last_step == 4);
}

TEST_CASE("no hits means no runs", "[events]") {
    CHECK(group_into_runs({}).empty());
}

TEST_CASE("candidate threshold covers the fastest possible pass between samples", "[events]") {
    CHECK_THAT(candidate_threshold_km(5.0, 10.0), WithinAbs(118.0, 1e-12));  // 5 + 22.4*5 + 1
    CHECK_THAT(candidate_threshold_km(5.0, 60.0), WithinAbs(678.0, 1e-12));  // 5 + 22.4*30 + 1
    CHECK_THROWS_AS(candidate_threshold_km(0.0, 10.0), std::invalid_argument);
    CHECK_THROWS_AS(candidate_threshold_km(5.0, 0.0), std::invalid_argument);
}

TEST_CASE("golden-section search finds the bottom of a valley", "[events]") {
    const auto valley = [](double t) { return (t - 3.7) * (t - 3.7) + 2.0; };
    CHECK_THAT(golden_section_minimize(valley, 0.0, 10.0, 1e-9), WithinAbs(3.7, 1e-6));
}

TEST_CASE("golden-section search handles the lowest point being at an end", "[events]") {
    const auto rising = [](double t) { return t; };
    CHECK_THAT(golden_section_minimize(rising, 2.0, 5.0, 1e-9), WithinAbs(2.0, 1e-6));
    const auto falling = [](double t) { return -t; };
    CHECK_THAT(golden_section_minimize(falling, 2.0, 5.0, 1e-9), WithinAbs(5.0, 1e-6));
}

TEST_CASE("golden-section search rejects bad input", "[events]") {
    const auto f = [](double t) { return t; };
    CHECK_THROWS_AS(golden_section_minimize(f, 5.0, 2.0, 1e-9), std::invalid_argument);
    CHECK_THROWS_AS(golden_section_minimize(f, 0.0, 1.0, 0.0), std::invalid_argument);
}

TEST_CASE("the skip rule: how far a closest sample can be and still matter", "[events]") {
    // 10 km/s, 10 s steps, 5 km miss: reach = 5 + (10 + 0.02*15) * 5 + 1 = 57.5 km.
    CHECK(can_get_within(57.0, 10.0, 10.0, 5.0));
    CHECK_FALSE(can_get_within(58.0, 10.0, 10.0, 5.0));
    // Two objects drifting side by side (0 km/s): reach = 5 + 0.3 * 5 + 1 = 7.5 km.
    CHECK(can_get_within(7.4, 0.0, 10.0, 5.0));
    CHECK_FALSE(can_get_within(7.6, 0.0, 10.0, 5.0));
    // Anything already inside the miss distance always counts.
    CHECK(can_get_within(4.0, 0.0, 10.0, 5.0));
}
