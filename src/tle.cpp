#include "kessler/tle.hpp"

#include <charconv>
#include <istream>
#include <cctype>
#include <cmath>

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

} // namespace

int tle_checksum(std::string_view line) {
    if (line.size() < 68){
	    throw TleParseError("tle_checksum: incorrect string");
    }
    int total = 0;
    for (char c : line.substr(0,68)){
	    if (std::isdigit(static_cast<unsigned char>(c))){
		    total += (c - '0');
	    }
	    else if (c == '-'){
		    total += 1;
	    }
    }
    total = total % 10;
    return total;
}

int tle_full_year(int two_digit_year) { 
    if (two_digit_year < 0 || two_digit_year > 99){
	    throw TleParseError("tle_full_year: invalid");
    }
    return (two_digit_year >= 57) ? (two_digit_year += 1900) : (two_digit_year += 2000);
}

double parse_implied_exponent(std::string_view f) {
    if (f.size() < 4){
	    throw TleParseError("parse_implied_exponent: invalid");
    }
    double sign = (f[0] ==  '-') ? -1.0 : 1.0;
    std::string_view mant = f.substr(1, f.size() - 3);
    int mantissa = 0;
    auto [p1, ec1] = std::from_chars(mant.data(), mant.data() + mant.size(), mantissa);
    if (ec1 != std::errc{}) {
	    throw TleParseError("parse_implied_exponent: invalid");
    }

    std::string_view expstr = f.substr(f.size() - 2);
    if (!expstr.empty() && (expstr.front()== '+' || expstr.front() == ' ')){
	    expstr.remove_prefix(1);
    }
    int exp = 0;
    auto [p2, ec2] = std::from_chars(expstr.data(), expstr.data() + expstr.size(), exp);
    if (ec2 != std::errc{}){
	    throw TleParseError("parse_implied_exponent: invalid");
    }
    int mantissa_d = static_cast<int>(mant.size());
    return sign * mantissa * std::pow(10.0, exp - mantissa_d);
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
