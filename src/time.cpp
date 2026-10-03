#include "kessler/time.hpp"

#include <cmath>
#include <stdexcept>

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

} // namespace kessler
