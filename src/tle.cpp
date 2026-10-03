#include "kessler/tle.hpp"

#include <charconv>
#include <istream>

namespace kessler {
namespace {

// Returns the characters in 1-indexed, inclusive columns [first, last].
// Example: field(line, 3, 7) on line 1 returns the catalog number
[[maybe_unused]] std::string_view field(std::string_view line, std::size_t first, std::size_t last) {
    if (last > line.size()) {
        throw TleParseError("TLE line too short");
    }
    return line.substr(first - 1, last - first + 1);
}

// TODO
//   - trim(std::string_view)       -> strip leading/trailing spaces
//   - to_double(std::string_view)  -> std::from_chars, throw TleParseError on failure
//   - to_int(std::string_view)     -> same for ints
// Hint: std::from_chars does NOT skip leading spaces and does NOT accept a
// leading '+'. Also note "-.00002182" has no digit before the '.', check that
// your number parsing accepts it.

} // namespace

int tle_checksum(std::string_view line) {
    // TODO(you)
    (void)line;
    throw TleParseError("tle_checksum: not implemented");
}

int tle_full_year(int two_digit_year) {
    // TODO(you)
    (void)two_digit_year;
    throw TleParseError("tle_full_year: not implemented");
}

double parse_implied_exponent(std::string_view f) {
    // TODO(you): " 12345-3" -> sign, mantissa digits, exponent sign + digit.
    (void)f;
    throw TleParseError("parse_implied_exponent: not implemented");
}

Tle parse_tle(std::string_view line1, std::string_view line2, std::string_view name) {
    // TODO(you), in this order:
    //   1. Validate: each line at least 69 chars, starts with '1' / '2'.
    //   2. Validate checksums (column 69) against tle_checksum().
    //   3. Validate both lines have the same catalog number.
    //   4. Fill every field of Tle using field(...) and the columns in tle.hpp.
    //      Watch out: eccentricity has an IMPLIED leading "0." ("0006703" -> 0.0006703).
    (void)line1; (void)line2; (void)name;
    throw TleParseError("parse_tle: not implemented");
}

std::vector<Tle> parse_tle_stream(std::istream& in) {
    // TODO(you): read lines; a line starting with '1' followed by one starting
    // with '2' is a TLE; a non-empty line right before them is the name.
    // Strip trailing '\r' (Windows line endings are common in downloaded files).
    (void)in;
    throw TleParseError("parse_tle_stream: not implemented");
}

} // namespace kessler
