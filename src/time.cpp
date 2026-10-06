#include "kessler/time.hpp"

#include <cmath>
#include <stdexcept>
#include <cstdio>

namespace kessler {

JulianDate julian_date(int year, int month, int day, int hour, int minute, double second) {
    if (year < 1900 || year > 2100) throw std::invalid_argument("julian_date: year outside 1900-2100");
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 ||
        minute > 59 || second < 0.0 || second >= 61.0) {
        throw std::invalid_argument("julian_date: field out of range");
    }
    const double y = year;
    const double m = month;
    // Vallado's jday() formula, valid 1900-2100 (2000 is a leap year, 1900 and
    // 2100 are not; the formula's simplified leap-year rule holds in between).
    const double jd0 = 367.0 * y - std::floor(7.0 * (y + std::floor((m + 9.0) / 12.0)) * 0.25) +
                       std::floor(275.0 * m / 9.0) + day + 1721013.5;
    const double frac = (hour * 3600.0 + minute * 60.0 + second) / 86400.0;
    return {jd0, frac};
}

JulianDate tle_epoch_jd(int year, double day_of_year) {
    if (day_of_year < 1.0 || day_of_year >= 367.0) {
        throw std::invalid_argument("tle_epoch_jd: day_of_year outside [1, 367)");
    }
    const double whole = std::floor(day_of_year);
    // "Jan 0" = Dec 31 of the previous year, so day 1.0 lands on Jan 1, 00:00.
    const double jan0 = julian_date(year, 1, 1).day - 1.0;
    return {jan0 + whole, day_of_year - whole};
}

double minutes_between(JulianDate from, JulianDate to) {
    return ((to.day - from.day) + (to.frac - from.frac)) * 1440.0;
}

JulianDate add_minutes(JulianDate t, double minutes){
    const double frac = t.frac + minutes / 1440.0;
    const double whole = std::floor(frac);
    return {t.day + whole, frac - whole};
}

std::string to_utc_string(JulianDate t) {
    const double whole = std::floor(t.frac);
    long long day_number = static_cast<long long>(t.day + 0.5 + whole);
    long long ms = std::llround((t.frac - whole) * 86400000.0);
    if (ms >= 86400000LL) {
        ms -= 86400000LL;
        ++day_number;
    }
    long long l = day_number + 68569;
    const long long n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    const long long i = 4000 * (l + 1) / 1461001; 
    l = l - 1461 * i / 4 + 31;
    const long long j = 80 * l / 2447;
    const long long day = l - 2447 * j / 80;
    l = j / 11;
    const long long month = j + 2 - 12 * l;
    const long long year = 100 * (n - 49) + i + l;

    char buf[96];
    std::snprintf(buf, sizeof buf, "%04lld-%02lld-%02lld %02lld:%02lld:%02lld.%03lld", year, month, day,
                  ms / 3600000, ms / 60000 % 60, ms / 1000 % 60, ms % 1000);
    return buf;
    }
} // namespace kessler
