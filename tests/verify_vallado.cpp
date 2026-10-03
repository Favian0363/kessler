// Differential verification against Vallado's reference pipeline.
//
// For every record in SGP4-VER.TLE, at every time from the record's own
// start/stop/step columns, compare:
//   reference: Vallado's twoline2rv() -> sgp4()
//   ours:      kessler::parse_tle() -> to_sgp4_elements() -> Propagator
// They must agree on epoch, on errors (same code, same time), and on
// position/velocity within 1 mm and 1 nm/s.
//
// Usage: verify_vallado <SGP4-VER.TLE> [i|a]

#include "kessler/propagator.hpp"
#include "kessler/sgp4_elements.hpp"
#include "kessler/tle.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace kessler;

namespace {

constexpr double kPosTolKm = 1e-6;    // 1 mm
constexpr double kVelTolKmS = 1e-9;   // 1 nm/s
constexpr double kEpochTolDays = 1e-9;

struct Record {
    std::string line1, line2;
    std::size_t line_no{};
};

std::vector<Record> read_records(std::ifstream& in) {
    std::vector<Record> out;
    std::string line, pending;
    std::size_t line_no = 0, pending_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.rfind("1 ", 0) == 0) {
            pending = line;
            pending_no = line_no;
        } else if (line.rfind("2 ", 0) == 0 && !pending.empty()) {
            out.push_back({pending, line, pending_no});
            pending.clear();
        }
    }
    return out;
}

// vallado's test file is wrong - ignores last checksum digit`
std::string with_fixed_checksum(std::string line) {
	line[68] = static_cast<char>('0' + tle_checksum(line));
	return line;
}

double max_abs_diff(const double* a, const std::array<double, 3>& b) {
    double m = 0.0;
    for (std::size_t i = 0; i < 3; ++i) m = std::max(m, std::fabs(a[i] - b[i]));
    return m;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <SGP4-VER.TLE> [i|a]\n", argv[0]);
        return 2;
    }
    const char mode_char = (argc > 2 && argv[2][0] == 'a') ? 'a' : 'i';
    const Sgp4Mode mode = mode_char == 'a' ? Sgp4Mode::afspc : Sgp4Mode::improved;

    std::ifstream in(argv[1]);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    const auto records = read_records(in);
    std::printf("%zu records, mode '%c'\n", records.size(), mode_char);

    int failures = 0, rejected = 0, samples = 0;
    double worst_dr = 0.0, worst_dv = 0.0, worst_epoch = 0.0;

    for (const auto& rec : records) {
        // ---- Reference path. twoline2rv edits its input, so pass copies. ----
        char s1[130] = {}, s2[130] = {};
        std::strncpy(s1, rec.line1.c_str(), sizeof s1 - 1);
        std::strncpy(s2, rec.line2.c_str(), sizeof s2 - 1);
        double start = 0, stop = 0, step = 0;
        elsetrec ref{};
        SGP4Funcs::twoline2rv(s1, s2, 'v', 'e', mode_char, wgs72, start, stop, step, ref);

        // ---- Our path. ----
        std::optional<Propagator> ours;
        std::string why;
        bool checksum_fixed = false;
        try {
            ours.emplace(to_sgp4_elements(parse_tle(rec.line1, rec.line2)), mode);
        } catch (const std::exception& first) {
            why = first.what();
            if (why.find("checksum") != std::string::npos) {
                // Only the checksum is wrong: retry with the digit corrected.
                try {
                    ours.emplace(to_sgp4_elements(parse_tle(with_fixed_checksum(rec.line1),
                                                           with_fixed_checksum(rec.line2))),
                                 mode);
                    checksum_fixed = true;
                } catch (const std::exception& second) {
                    why = second.what();
                }
            }
        }
        const std::string id = rec.line1.substr(2, 5);
        if (!ours) {
            if (ref.error != 0) {
                std::printf("  %s: both rejected at init (ref error %d) - agree\n", id.c_str(), ref.error);
            } else {
                std::printf("  %s: REJECTED by kessler only (line %zu): %s\n", id.c_str(), rec.line_no, why.c_str());
                ++rejected;
            }
            continue;
        }
        if (ref.error != 0) {
            std::printf("  %s: FAIL - reference rejected at init (error %d), kessler accepted\n", id.c_str(), ref.error);
            ++failures;
            continue;
        }

        const double epoch_diff = std::fabs((ref.jdsatepoch + ref.jdsatepochF) - ours->epoch().total());
        worst_epoch = std::max(worst_epoch, epoch_diff);
        if (epoch_diff > kEpochTolDays) {
            std::printf("  %s: FAIL - epoch differs by %.3e days\n", id.c_str(), epoch_diff);
            ++failures;
            continue;
        }

        int n = 0;
        double rec_dr = 0.0, rec_dv = 0.0;
        bool ok_record = true;
        const int max_steps = step > 0 ? static_cast<int>(std::floor((stop - start) / step + 1e-9)) : 0;
        for (int k = 0; k <= max_steps; ++k) {
            const double t = start + k * step;
            double rr[3], rv[3];
            const bool ref_ok = SGP4Funcs::sgp4(ref, t, rr, rv) && ref.error == 0;
            StateVector sv;
            const bool our_ok = ours->propagate(t, sv);

            if (ref_ok != our_ok || (!ref_ok && ref.error != ours->error_code())) {
                std::printf("  %s: FAIL at t=%.2f min - ref ok=%d err=%d, kessler ok=%d err=%d\n",
                            id.c_str(), t, ref_ok, ref.error, our_ok, ours->error_code());
                ok_record = false;
                break;
            }
            if (!ref_ok) break;  // both stopped with the same error (e.g. decay): agreement

            const double dr = max_abs_diff(rr, sv.r_km);
            const double dv = max_abs_diff(rv, sv.v_km_s);
            rec_dr = std::max(rec_dr, dr);
            rec_dv = std::max(rec_dv, dv);
            ++n;
            if (dr > kPosTolKm || dv > kVelTolKmS) {
                std::printf("  %s: FAIL at t=%.2f min - |dr|=%.3e km |dv|=%.3e km/s\n", id.c_str(), t, dr, dv);
                ok_record = false;
                break;
            }
        }
        samples += n;
        worst_dr = std::max(worst_dr, rec_dr);
        worst_dv = std::max(worst_dv, rec_dv);
        if (ok_record) {
            std::printf("  %s: %3d samples  max|dr|=%.2e km  max|dv|=%.2e km/s%s\n", id.c_str(), n,
                        rec_dr, rec_dv, checksum_fixed ? "  (bad checksum digit fixed for comparison)" : "");
	} else {
            ++failures;
        }
    }

    std::printf("\n%d samples compared. worst |dr| = %.3e km, worst |dv| = %.3e km/s, worst epoch diff = %.3e days\n",
                samples, worst_dr, worst_dv, worst_epoch);
    std::printf("%d failures, %d records rejected by kessler only\n", failures, rejected);
    return failures == 0 ? 0 : 1;
}
