#pragma once
// Time handling.
//
// A Julian Date (JD) counts days continuously from 4713 BC, so the time
// between two instants is a plain subtraction, with no months, leap years,
// or time zones involved. All times here are UTC, which is what TLE epochs use.
//
// We split a JD into a whole-day part and a fraction, exactly as Vallado's
// SGP4 does. A single double near JD 2.45 million resolves only ~40 microseconds,
// harmless for screening (well under a millimeter of motion), but matching the
// reference representation lets our results match the reference test vectors.

namespace kessler {

struct JulianDate {
    double day{};   // JD at 00:00 UTC of the date (always ends in .5)
    double frac{};  // fraction of the day elapsed since 00:00 UTC, in [0, 1)

    [[nodiscard]] double total() const { return day + frac; }
};

/// Calendar date and UTC time -> Julian Date. Valid for 1900-2100.
/// Throws std::invalid_argument outside that range or for impossible fields.
JulianDate julian_date(int year, int month, int day, int hour = 0, int minute = 0,
                       double second = 0.0);

/// TLE epoch -> Julian Date. `day_of_year` follows the TLE convention:
/// Jan 1, 00:00 UTC is 1.0 (NOT 0.0); a one-day error here moves a satellite
/// to a completely different point in its orbit.
JulianDate tle_epoch_jd(int year, double day_of_year);

/// Minutes from `from` to `to` (positive if `to` is later). Whole days are
/// subtracted first so the fractional parts keep full precision.
double minutes_between(JulianDate from, JulianDate to);

} // namespace kessler
