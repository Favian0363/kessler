// Unit tests for TLE parsing.
// Fixture: the ISS TLE used as the example in the TLE format documentation.
// Every expected value below was checked column-by-column against the spec.

#include "kessler/tle.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <sstream>
#include <string>

using Catch::Matchers::WithinAbs;
using namespace kessler;

namespace {
const std::string kIss1 =
    "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927";
const std::string kIss2 =
    "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537";
} // namespace

TEST_CASE("checksum matches column 69", "[tle]") {
    CHECK(tle_checksum(kIss1) == 7);
    CHECK(tle_checksum(kIss2) == 7);
}

TEST_CASE("two-digit year pivot at 57", "[tle]") {
    CHECK(tle_full_year(57) == 1957);
    CHECK(tle_full_year(99) == 1999);
    CHECK(tle_full_year(0) == 2000);
    CHECK(tle_full_year(8) == 2008);
    CHECK(tle_full_year(56) == 2056);
}

TEST_CASE("implied-decimal exponent fields", "[tle]") {
    CHECK_THAT(parse_implied_exponent("-11606-4"), WithinAbs(-0.11606e-4, 1e-15));
    CHECK_THAT(parse_implied_exponent(" 12345-3"), WithinAbs(0.12345e-3, 1e-15));
    CHECK_THAT(parse_implied_exponent(" 00000-0"), WithinAbs(0.0, 1e-15));
    CHECK_THAT(parse_implied_exponent("+50000+1"), WithinAbs(5.0, 1e-12));
}

TEST_CASE("parses every field of the ISS TLE", "[tle]") {
    const Tle t = parse_tle(kIss1, kIss2, "ISS (ZARYA)");

    CHECK(t.name == "ISS (ZARYA)");
    CHECK(t.catalog_number == 25544);
    CHECK(t.classification == 'U');
    CHECK(t.intl_designator == "98067A");
    CHECK(t.epoch_year == 2008);
    CHECK_THAT(t.epoch_day,     WithinAbs(264.51782528, 1e-9));
    CHECK_THAT(t.ndot_over_2,   WithinAbs(-0.00002182, 1e-12));
    CHECK_THAT(t.nddot_over_6,  WithinAbs(0.0, 1e-15));
    CHECK_THAT(t.bstar,         WithinAbs(-0.11606e-4, 1e-15));
    CHECK(t.element_set_number == 292);

    CHECK_THAT(t.inclination_deg,         WithinAbs(51.6416, 1e-9));
    CHECK_THAT(t.raan_deg,                WithinAbs(247.4627, 1e-9));
    CHECK_THAT(t.eccentricity,            WithinAbs(0.0006703, 1e-12));
    CHECK_THAT(t.arg_perigee_deg,         WithinAbs(130.5360, 1e-9));
    CHECK_THAT(t.mean_anomaly_deg,        WithinAbs(325.0288, 1e-9));
    CHECK_THAT(t.mean_motion_rev_per_day, WithinAbs(15.72125391, 1e-9));
    CHECK(t.rev_number == 56353);
}

TEST_CASE("rejects corrupted input", "[tle]") {
    SECTION("bad checksum") {
        std::string bad = kIss1;
        bad[68] = '0';
        CHECK_THROWS_AS(parse_tle(bad, kIss2), TleParseError);
    }
    SECTION("line too short") {
        CHECK_THROWS_AS(parse_tle(kIss1.substr(0, 40), kIss2), TleParseError);
    }
    SECTION("lines swapped") {
        CHECK_THROWS_AS(parse_tle(kIss2, kIss1), TleParseError);
    }
    SECTION("catalog numbers disagree") {
        // Change the catalog number on line 2 AND fix its checksum, so the
        // only problem is the mismatch. (25544 -> 25545 adds 1 to the digit sum.)
        std::string other = kIss2;
        other[6] = '5';
        other[68] = '8';
        CHECK_THROWS_AS(parse_tle(kIss1, other), TleParseError);
    }
}

TEST_CASE("parses 2-line and 3-line streams with CRLF endings", "[tle]") {
    std::istringstream three("ISS (ZARYA)\r\n" + kIss1 + "\r\n" + kIss2 + "\r\n");
    const auto a = parse_tle_stream(three);
    REQUIRE(a.size() == 1);
    CHECK(a[0].name == "ISS (ZARYA)");

    std::istringstream two(kIss1 + "\n" + kIss2 + "\n");
    const auto b = parse_tle_stream(two);
    REQUIRE(b.size() == 1);
    CHECK(b[0].name.empty());
    CHECK(b[0].catalog_number == 25544);
}
