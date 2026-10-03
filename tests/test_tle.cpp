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

// ---------------------------------------------------------------------------
// Regression tests: real-world failure modes found in code review.
// ---------------------------------------------------------------------------

namespace {
// Rewrites column 69 so the ONLY defect in a line is the one a test introduces.
std::string with_checksum(std::string line) {
    line[68] = static_cast<char>('0' + tle_checksum(line));
    return line;
}

// Returns the exception message, or "" if nothing was thrown.
template <typename F>
std::string error_of(F&& f) {
    try { f(); } catch (const TleParseError& e) { return e.what(); }
    return "";
}
} // namespace

TEST_CASE("accepts a '+' sign in the ndot field", "[tle][regression]") {
    std::string l1 = kIss1;
    l1[33] = '+';
    const Tle t = parse_tle(with_checksum(l1), kIss2);
    CHECK_THAT(t.ndot_over_2, WithinAbs(0.00002182, 1e-12));
}

TEST_CASE("garbage inside a number throws and names the field", "[tle][regression]") {
    std::string l2 = kIss2;
    l2[12] = 'x';  // " 51.6416" -> " 51.x416"
    const std::string msg = error_of([&] { parse_tle(kIss1, with_checksum(l2)); });
    CHECK(msg.find("inclination") != std::string::npos);
}

TEST_CASE("implied-exponent fields are validated strictly", "[tle][regression]") {
    CHECK_THROWS_AS(parse_implied_exponent("11606-4"), TleParseError);   // 7 chars
    CHECK_THROWS_AS(parse_implied_exponent("x11606-4"), TleParseError);  // bad sign column
    CHECK_THROWS_AS(parse_implied_exponent("-11a06-4"), TleParseError);  // non-digit mantissa
}

TEST_CASE("orphan line 2 is reported, never used as a name", "[tle][regression]") {
    const std::string text = kIss2 + "\n" + kIss1 + "\n" + kIss2 + "\n";

    std::istringstream strict(text);
    CHECK(error_of([&] { parse_tle_stream(strict); }).find("line 1:") != std::string::npos);

    std::istringstream lenient(text);
    const auto r = parse_tle_stream_lenient(lenient);
    REQUIRE(r.tles.size() == 1);
    CHECK(r.tles[0].name.empty());
    CHECK(r.errors.size() == 1);
}

TEST_CASE("line 1 without a line 2 is reported", "[tle][regression]") {
    std::istringstream lenient(kIss1 + "\n" + kIss1 + "\n" + kIss2 + "\n");
    const auto r = parse_tle_stream_lenient(lenient);
    CHECK(r.tles.size() == 1);
    CHECK(r.errors.size() == 1);

    std::istringstream at_eof(kIss1 + "\n");
    CHECK_THROWS_AS(parse_tle_stream(at_eof), TleParseError);
}

TEST_CASE("errors carry the line number of the bad record", "[tle][regression]") {
    std::string bad = kIss2;
    bad[68] = '0';
    std::string text;
    for (int i = 0; i < 3; ++i) text += kIss1 + "\n" + (i == 2 ? bad : kIss2) + "\n";
    std::istringstream in(text);
    CHECK(error_of([&] { parse_tle_stream(in); }).find("line 5:") != std::string::npos);
}

TEST_CASE("Space-Track '0 ' name prefix and comments are handled", "[tle][regression]") {
    std::istringstream in("# a comment\n0 ISS (ZARYA)\n" + kIss1 + "\n" + kIss2 + "\n");
    const auto tles = parse_tle_stream(in);
    REQUIRE(tles.size() == 1);
    CHECK(tles[0].name == "ISS (ZARYA)");
}
