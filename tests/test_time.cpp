// Expected values computed independently with the Fliegel-Van Flandern
// algorithm (a different method from the one under test).

#include "kessler/time.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <stdexcept>

using Catch::Matchers::WithinAbs;
using namespace kessler;

TEST_CASE("calendar dates convert to known Julian Dates", "[time]") {
    const JulianDate j2000 = julian_date(2000, 1, 1, 12);
    CHECK_THAT(j2000.day, WithinAbs(2451544.5, 0.0));
    CHECK_THAT(j2000.frac, WithinAbs(0.5, 0.0));
    CHECK_THAT(julian_date(2008, 1, 1).total(), WithinAbs(2454466.5, 0.0));
    CHECK_THAT(julian_date(1950, 1, 1).day - 1.0, WithinAbs(2433281.5, 0.0));  // 1950 Jan 0.0
}

TEST_CASE("TLE epoch: Jan 1 is day 1.0, not 0.0", "[time]") {
    const JulianDate jan1 = tle_epoch_jd(2008, 1.0);
    CHECK_THAT(jan1.total(), WithinAbs(julian_date(2008, 1, 1).total(), 0.0));
}

TEST_CASE("ISS TLE epoch is 2008-09-20 12:25:40 UTC", "[time]") {
    const JulianDate e = tle_epoch_jd(2008, 264.51782528);
    CHECK_THAT(e.day, WithinAbs(2454729.5, 0.0));
    CHECK_THAT(e.frac, WithinAbs(0.51782528, 1e-12));
    CHECK_THAT(e.total(), WithinAbs(2454730.01782528, 1e-8));
}

TEST_CASE("leap years: day 366 exists in 2024", "[time]") {
    CHECK_THAT(tle_epoch_jd(2024, 366.0).total(),
               WithinAbs(julian_date(2024, 12, 31).total(), 0.0));
}

TEST_CASE("minutes_between keeps sub-microsecond precision", "[time]") {
    const JulianDate a = tle_epoch_jd(2008, 264.51782528);
    const JulianDate b{a.day + 1.0, a.frac};
    CHECK_THAT(minutes_between(a, b), WithinAbs(1440.0, 1e-9));
    CHECK_THAT(minutes_between(b, a), WithinAbs(-1440.0, 1e-9));
    const JulianDate c{a.day, a.frac + 1.0 / 1440.0};  // one minute later
    CHECK_THAT(minutes_between(a, c), WithinAbs(1.0, 1e-8));
}

TEST_CASE("invalid dates are rejected", "[time]") {
    CHECK_THROWS_AS(julian_date(1899, 12, 31), std::invalid_argument);
    CHECK_THROWS_AS(julian_date(2026, 13, 1), std::invalid_argument);
    CHECK_THROWS_AS(tle_epoch_jd(2026, 0.5), std::invalid_argument);
}
