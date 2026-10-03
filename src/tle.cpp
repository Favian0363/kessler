#include "kessler/tle.hpp"

#include <array>
#include <charconv>
#include <istream>
#include <string>

namespace kessler {
namespace {

// Characters in 1-indexed, inclusive columns [first, last], matching the TLE spec.
std::string_view field(std::string_view line, std::size_t first, std::size_t last) {
    if (last > line.size()) {
        throw TleParseError("line too short for columns " + std::to_string(first) + "-" +
                            std::to_string(last));
    }
    return line.substr(first - 1, last - first + 1);
}

std::string_view trim(std::string_view sv) {
    while (!sv.empty() && sv.front() == ' ') sv.remove_prefix(1);
    while (!sv.empty() && sv.back() == ' ') sv.remove_suffix(1);
    return sv;
}

[[noreturn]] void bad_field(std::string_view what, std::string_view raw) {
    throw TleParseError(std::string(what) + ": invalid value '" + std::string(raw) + "'");
}

// Strict integer parse: the WHOLE (trimmed) field must be a number.
int to_int(std::string_view raw, std::string_view what) {
    std::string_view sv = trim(raw);
    if (!sv.empty() && sv.front() == '+') sv.remove_prefix(1);
    int value{};
    const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (sv.empty() || ec != std::errc{} || ptr != sv.data() + sv.size()) bad_field(what, raw);
    return value;
}

// Strict floating-point parse. from_chars rejects a leading '+', so strip it first;
// it accepts "-.00002182" (no digit before the point), which TLEs use.
double to_double(std::string_view raw, std::string_view what) {
    std::string_view sv = trim(raw);
    if (!sv.empty() && sv.front() == '+') sv.remove_prefix(1);
    double value{};
    const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (sv.empty() || ec != std::errc{} || ptr != sv.data() + sv.size()) bad_field(what, raw);
    return value;
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }

bool is_tle_line(std::string_view line, char number) {
    return line.size() >= 2 && line[0] == number && line[1] == ' ';
}

} // namespace

int tle_checksum(std::string_view line) {
    if (line.size() < 68) throw TleParseError("checksum: line shorter than 68 characters");
    int total = 0;
    for (const char c : line.substr(0, 68)) {
        if (is_digit(c)) total += c - '0';
        else if (c == '-') total += 1;
    }
    return total % 10;
}

int tle_full_year(int two_digit_year) {
    if (two_digit_year < 0 || two_digit_year > 99) {
        throw TleParseError("epoch year out of range: " + std::to_string(two_digit_year));
    }
    return two_digit_year >= 57 ? 1900 + two_digit_year : 2000 + two_digit_year;
}

// Format: [sign][5 digits][exp sign][exp digit], e.g. "-11606-4" = -0.11606e-4.
// Rebuilt as the string "-0.11606e-4" and parsed once, so the result is
// correctly rounded rather than the product of several floating-point steps.
double parse_implied_exponent(std::string_view f) {
    const auto sign_ok = [](char c) { return c == ' ' || c == '+' || c == '-'; };
    if (f.size() != 8 || !sign_ok(f[0]) || !sign_ok(f[6]) || !is_digit(f[7])) {
        bad_field("implied-exponent field", f);
    }
    for (std::size_t i = 1; i <= 5; ++i) {
        if (!is_digit(f[i])) bad_field("implied-exponent field", f);
    }
    std::array<char, 12> buf{};
    std::size_t n = 0;
    if (f[0] == '-') buf[n++] = '-';
    buf[n++] = '0';
    buf[n++] = '.';
    for (std::size_t i = 1; i <= 5; ++i) buf[n++] = f[i];
    buf[n++] = 'e';
    buf[n++] = (f[6] == '-') ? '-' : '+';
    buf[n++] = f[7];
    double value{};
    const auto [ptr, ec] = std::from_chars(buf.data(), buf.data() + n, value);
    if (ec != std::errc{} || ptr != buf.data() + n) bad_field("implied-exponent field", f);
    return value;
}

Tle parse_tle(std::string_view line1, std::string_view line2, std::string_view name) {
    if (line1.size() < 69 || line2.size() < 69) {
        throw TleParseError("line shorter than 69 characters");
    }
    if (line1[0] != '1' || line2[0] != '2') {
        throw TleParseError("lines must start with '1' and '2'");
    }
    for (const auto& [line, label] : {std::pair{line1, "line 1"}, std::pair{line2, "line 2"}}) {
        if (!is_digit(line[68])) {
            throw TleParseError(std::string(label) + ": checksum column is not a digit");
        }
        if (line[68] - '0' != tle_checksum(line)) {
            throw TleParseError(std::string(label) + ": checksum mismatch");
        }
    }
    if (field(line1, 3, 7) != field(line2, 3, 7)) {
        throw TleParseError("catalog numbers differ between line 1 and line 2");
    }

    Tle t;
    t.name = std::string(trim(name));

    // ---- Line 1 ----
    t.catalog_number     = to_int(field(line1, 3, 7), "catalog_number (cols 3-7)");
    t.classification     = line1[7];
    t.intl_designator    = std::string(trim(field(line1, 10, 17)));
    t.epoch_year         = tle_full_year(to_int(field(line1, 19, 20), "epoch_year (cols 19-20)"));
    t.epoch_day          = to_double(field(line1, 21, 32), "epoch_day (cols 21-32)");
    t.ndot_over_2        = to_double(field(line1, 34, 43), "ndot_over_2 (cols 34-43)");
    t.nddot_over_6       = parse_implied_exponent(field(line1, 45, 52));
    t.bstar              = parse_implied_exponent(field(line1, 54, 61));
    t.element_set_number = to_int(field(line1, 65, 68), "element_set_number (cols 65-68)");

    // ---- Line 2 ----
    t.inclination_deg  = to_double(field(line2, 9, 16), "inclination (cols 9-16)");
    t.raan_deg         = to_double(field(line2, 18, 25), "raan (cols 18-25)");
    // Implied leading "0.": seven digits -> integer / 1e7 is exact for this range.
    t.eccentricity     = to_int(field(line2, 27, 33), "eccentricity (cols 27-33)") / 1e7;
    t.arg_perigee_deg  = to_double(field(line2, 35, 42), "arg_perigee (cols 35-42)");
    t.mean_anomaly_deg = to_double(field(line2, 44, 51), "mean_anomaly (cols 44-51)");
    t.mean_motion_rev_per_day = to_double(field(line2, 53, 63), "mean_motion (cols 53-63)");
    t.rev_number       = to_int(field(line2, 64, 68), "rev_number (cols 64-68)");

    if (t.epoch_day < 1.0 || t.epoch_day >= 367.0) {
        throw TleParseError("epoch_day out of range");
    }
    return t;
}

namespace {

// One state machine for both modes. In strict mode the first problem throws;
// in lenient mode problems are recorded and parsing continues.
TleStreamResult parse_stream_impl(std::istream& in, bool strict) {
    TleStreamResult out;
    std::string line, name, line1;
    std::size_t line_no = 0, line1_no = 0;

    const auto issue = [&](std::size_t at, const std::string& msg) {
        std::string full = "line " + std::to_string(at) + ": " + msg;
        if (strict) throw TleParseError(full);
        out.errors.push_back(std::move(full));
    };

    while (std::getline(in, line)) {
        ++line_no;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty() || line.front() == '#') continue;  // blank lines and comments

        if (is_tle_line(line, '1')) {
            if (!line1.empty()) issue(line1_no, "line 1 not followed by a line 2");
            line1 = line;
            line1_no = line_no;
        } else if (is_tle_line(line, '2')) {
            if (line1.empty()) {
                issue(line_no, "line 2 without a preceding line 1");
            } else {
                try {
                    out.tles.push_back(parse_tle(line1, line, name));
                } catch (const TleParseError& e) {
                    issue(line1_no, e.what());
                }
            }
            line1.clear();
            name.clear();
        } else {
            if (!line1.empty()) {
                issue(line1_no, "line 1 not followed by a line 2");
                line1.clear();
            }
            std::string_view nm = line;
            if (nm.size() > 2 && nm[0] == '0' && nm[1] == ' ') nm.remove_prefix(2);  // Space-Track 3LE
            name = std::string(trim(nm));
        }
    }
    if (!line1.empty()) issue(line1_no, "line 1 not followed by a line 2 (end of file)");
    return out;
}

} // namespace

std::vector<Tle> parse_tle_stream(std::istream& in) {
    return parse_stream_impl(in, /*strict=*/true).tles;
}

TleStreamResult parse_tle_stream_lenient(std::istream& in) {
    return parse_stream_impl(in, /*strict=*/false);
}

} // namespace kessler
