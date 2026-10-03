#pragma once
// TLE (Two-Line Element set) parsing.
//
// A TLE is a fixed-width text format: every field lives at fixed COLUMNS.
// Column numbers below are 1-indexed to match the official spec, so you can
// compare this file against any TLE reference side by side.
//
// Reference: https://celestrak.org/columns/v04n03/

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kessler {

struct TleParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Tle {
    std::string name;                 // optional "line 0" (e.g. "ISS (ZARYA)")

    // ---- Line 1 ----
    int         catalog_number{};     // cols  3-7   NORAD catalog number
    char        classification{};     // col   8     U = unclassified
    std::string intl_designator;      // cols 10-17  launch year/number/piece (trimmed)
    int         epoch_year{};         // cols 19-20  -> converted to 4 digits
    double      epoch_day{};          // cols 21-32  day of year; 1.0 = Jan 1, 00:00 UTC
    double      ndot_over_2{};        // cols 34-43  1st derivative of mean motion / 2 [rev/day^2]
    double      nddot_over_6{};       // cols 45-52  2nd derivative of mean motion / 6 [rev/day^3]
    double      bstar{};              // cols 54-61  drag term [1/earth radii]
    int         element_set_number{}; // cols 65-68

    // ---- Line 2 ----
    double inclination_deg{};         // cols  9-16
    double raan_deg{};                // cols 18-25  right ascension of ascending node
    double eccentricity{};            // cols 27-33  implied leading decimal point
    double arg_perigee_deg{};         // cols 35-42
    double mean_anomaly_deg{};        // cols 44-51
    double mean_motion_rev_per_day{}; // cols 53-63
    int    rev_number{};              // cols 64-68  revolution count at epoch
};

/// Checksum over the first 68 characters: sum of all digits, each '-' counts
/// as 1, everything else counts as 0; result modulo 10.
int tle_checksum(std::string_view line);

/// TLEs store 2-digit years. Convention: 57-99 -> 1957-1999, 00-56 -> 2000-2056.
int tle_full_year(int two_digit_year);

/// Parses fields written with an implied leading decimal and an exponent,
/// e.g. " 12345-3" means +0.12345e-3 and "-11606-4" means -0.11606e-4.
/// Used for nddot_over_6 and bstar.
double parse_implied_exponent(std::string_view field);

/// Parses one TLE. Throws TleParseError if lines are malformed, have a bad
/// checksum, or describe different satellites.
Tle parse_tle(std::string_view line1, std::string_view line2, std::string_view name = {});

/// Parses a whole file in 2-line or 3-line (with names) format. Blank lines
/// and lines starting with '#' are skipped; Space-Track's "0 " name prefix is
/// removed. Strict: throws on the first problem, with the line number.
std::vector<Tle> parse_tle_stream(std::istream& in);

struct TleStreamResult {
    std::vector<Tle> tles;
    std::vector<std::string> errors;  // "line N: what went wrong"
};

/// Same as parse_tle_stream, but skips bad entries and reports them instead of
/// throwing. Use for real catalogs, where one corrupt record must not abort a
/// 30,000-object run.
TleStreamResult parse_tle_stream_lenient(std::istream& in);

} // namespace kessler
