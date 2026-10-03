// Expected values computed independently in Python from the ISS TLE.

#include "kessler/sgp4_elements.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using namespace kessler;

TEST_CASE("ISS TLE converts to SGP4 units", "[sgp4]") {
    const Tle t = parse_tle(
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927",
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537");
    const Sgp4Elements e = to_sgp4_elements(t);

    CHECK(e.catalog_number == 25544);
    CHECK_THAT(e.epoch_days_since_1950, WithinAbs(21448.51782528, 1e-8));
    CHECK_THAT(e.mean_motion_rad_per_min, WithinAbs(0.068596910817883, 1e-15));
    CHECK_THAT(e.inclination_rad, WithinAbs(0.901315950997904, 1e-15));
    CHECK_THAT(e.ndot, WithinAbs(-6.611646576131297e-11, 1e-24));
    CHECK_THAT(e.eccentricity, WithinAbs(0.0006703, 1e-15));
    CHECK_THAT(e.bstar, WithinAbs(-0.11606e-4, 1e-18));
}
