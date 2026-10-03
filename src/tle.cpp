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

std::string_view trim_left(std::string_view sv){
	while (!sv.empty() && sv.front() == ' ') {
		sv.remove_prefix(1);
	}
	return sv;
}

std::string_view trim_right(std::string_view sv){
	while (!sv.empty() && sv.back() == ' ') {
		sv.remove_suffix(1);
	}
	return sv;
}

std::string_view trim(std::string_view sv) {
	return trim_right(trim_left(sv));
}

int parse_int(std::string_view sv){
	sv = trim(sv);
	if (sv.empty()){
		throw TleParseError("Expected int, got string");
	}
	int val = 0;
	std::from_chars(sv.data(), sv.data() + sv.size(), val);
	return val;
}

double parse_double(std::string_view sv){
	sv = trim(sv);
	if (sv.empty()){
		throw TleParseError("Expected double, got string");
	}
	double val = 0.0;
	std::from_chars(sv.data(), sv.data() + sv.size(), val);
	return val;
}

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
    if (line1.size() < 69 || line2.size() < 69) {
        throw TleParseError("parse_tle: line too short");
    }

    if (line1[0] != '1' || line2[0] != '2') {
        throw TleParseError("parse_tle: invalid line prefix");
    }

    if ((line1[68] - '0') != tle_checksum(line1) || (line2[68] - '0') != tle_checksum(line2)) {
        throw TleParseError("parse_tle: invalid checksum");
    }

    if (field(line1, 3, 7) != field(line2, 3, 7)) {
        throw TleParseError("parse_tle: catalog numbers do not match");
    }

    Tle tle;
    tle.name = std::string(trim(name));

    // Line 1
    tle.catalog_number = parse_int(field(line1, 3, 7));
    tle.classification = line1[7]; // Col 8
    tle.intl_designator = std::string(trim(field(line1, 10, 17)));
    tle.epoch_year = tle_full_year(parse_int(field(line1, 19, 20)));
    tle.epoch_day = parse_double(field(line1, 21, 32));
    tle.ndot_over_2 = parse_double(field(line1, 34, 43));
    tle.nddot_over_6 = parse_implied_exponent(field(line1, 45, 52));
    tle.bstar = parse_implied_exponent(field(line1, 54, 61));
    tle.element_set_number = parse_int(field(line1, 65, 68));

    // Line 2
    tle.inclination_deg = parse_double(field(line2, 9, 16));
    tle.raan_deg = parse_double(field(line2, 18, 25));
    tle.eccentricity = parse_int(field(line2, 27, 33)) / 1e7;
    tle.arg_perigee_deg = parse_double(field(line2, 35, 42));
    tle.mean_anomaly_deg = parse_double(field(line2, 44, 51));
    tle.mean_motion_rev_per_day = parse_double(field(line2, 53, 63));
    tle.rev_number = parse_int(field(line2, 64, 68));

    return tle;
}

std::vector<Tle> parse_tle_stream(std::istream& in) {
    std::vector<Tle> result;
    std::string line;
    std::string current_name;
    std::string line1;

    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }

        std::string_view sv = trim_right(line);
        if (sv.empty()) {
            continue;
        }

        if (sv.size() >= 69 && sv[0] == '1') {
            line1 = std::string(sv);
        } else if (sv.size() >= 69 && sv[0] == '2' && !line1.empty()) {
            result.push_back(parse_tle(line1, sv, current_name));
            line1.clear();
            current_name.clear();
        } else {
            current_name = std::string(trim(sv));
        }
    }

    return result;
}

} // namespace kessler
