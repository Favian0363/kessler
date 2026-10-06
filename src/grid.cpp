#include "kessler/grid.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace kessler {
namespace {

// Each cell coordinate is stored in 21 bits, shifted up so it is never negative.
constexpr std::int64_t kBits = 21;
constexpr std::int64_t kOffset = std::int64_t{1} << (kBits - 1);  // 1,048,576
constexpr std::int64_t kMaxCell = (std::int64_t{1} << kBits) - 1;  // 2,097,151

// Which cell a coordinate falls in (shifted to be >= 0).
std::int64_t cell_of(double coordinate_km, double cell_km) {
    const double c = std::floor(coordinate_km / cell_km) + static_cast<double>(kOffset);
    if (!(c >= 0.0 && c <= static_cast<double>(kMaxCell))) {
        throw std::out_of_range("find_close_pairs_grid: position too far out for the grid");
    }
    return static_cast<std::int64_t>(c);
}

// One number per cell. z goes in the lowest bits, so the cells (x, y, z-1),
// (x, y, z) and (x, y, z+1) get consecutive numbers: a single search in the
// sorted list finds all three at once.
std::uint64_t cell_key(std::int64_t cx, std::int64_t cy, std::int64_t cz) {
    return (static_cast<std::uint64_t>(cx) << (2 * kBits)) |
           (static_cast<std::uint64_t>(cy) << kBits) | static_cast<std::uint64_t>(cz);
}

struct Entry {
    std::uint64_t key;    // which cell
    std::uint32_t index;  // which object
};

}  // namespace

void find_close_pairs_grid(const std::vector<Vec3>& positions, const std::vector<char>& alive,
                           double threshold_km, int step, std::vector<Hit>& out) {
    if (positions.size() != alive.size()) {
        throw std::invalid_argument("find_close_pairs_grid: positions and alive differ in size");
    }
    if (!(threshold_km > 0.0)) throw std::invalid_argument("find_close_pairs_grid: threshold must be > 0");

    // Cells a hair wider than the threshold, so rounding in floor(x / cell)
    // can never put two close objects two cells apart.
    const double cell = threshold_km * (1.0 + 1e-9);
    const double t2 = threshold_km * threshold_km;
    const std::size_t n = positions.size();

    // 1. Work out every alive object's cell.
    std::vector<std::int64_t> cx(n), cy(n), cz(n);
    std::vector<Entry> entries;
    entries.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (!alive[i]) continue;
        cx[i] = cell_of(positions[i].x, cell);
        cy[i] = cell_of(positions[i].y, cell);
        cz[i] = cell_of(positions[i].z, cell);
        entries.push_back({cell_key(cx[i], cy[i], cz[i]), static_cast<std::uint32_t>(i)});
    }

    // 2. Sort by cell, so the objects in each cell sit next to each other.
    std::sort(entries.begin(), entries.end(),
              [](const Entry& p, const Entry& q) { return p.key < q.key; });

    // 3. For each object, search the 9 columns of cells around it (each column
    //    is 3 cells stacked in z). Threads split the objects between them;
    //    each thread collects its hits separately, then they are merged.
    std::vector<Hit> step_hits;
    const auto count = static_cast<std::int64_t>(entries.size());
#pragma omp parallel
    {
        std::vector<Hit> local;
#pragma omp for schedule(dynamic, 256) nowait
        for (std::int64_t e = 0; e < count; ++e) {
            const std::uint32_t i = entries[static_cast<std::size_t>(e)].index;
            const Vec3& p = positions[i];
            for (std::int64_t dx = -1; dx <= 1; ++dx) {
                for (std::int64_t dy = -1; dy <= 1; ++dy) {
                    const std::int64_t nx = cx[i] + dx;
                    const std::int64_t ny = cy[i] + dy;
                    if (nx < 0 || nx > kMaxCell || ny < 0 || ny > kMaxCell) continue;
                    const std::int64_t z_lo = std::max<std::int64_t>(cz[i] - 1, 0);
                    const std::int64_t z_hi = std::min<std::int64_t>(cz[i] + 1, kMaxCell);
                    const auto first = std::lower_bound(
                        entries.begin(), entries.end(), cell_key(nx, ny, z_lo),
                        [](const Entry& en, std::uint64_t k) { return en.key < k; });
                    const auto last = std::upper_bound(
                        first, entries.end(), cell_key(nx, ny, z_hi),
                        [](std::uint64_t k, const Entry& en) { return k < en.key; });
                    for (auto it = first; it != last; ++it) {
                        const std::uint32_t j = it->index;
                        if (j <= i) continue;  // each pair once: only from its smaller index
                        const double ddx = p.x - positions[j].x;
                        const double ddy = p.y - positions[j].y;
                        const double ddz = p.z - positions[j].z;
                        const double d2 = ddx * ddx + ddy * ddy + ddz * ddz;
                        if (d2 < t2) local.push_back({i, j, step, std::sqrt(d2)});
                    }
                }
            }
        }
#pragma omp critical
        step_hits.insert(step_hits.end(), local.begin(), local.end());
    }

    // 4. Put the hits in the same order brute force uses: by a, then b.
    std::sort(step_hits.begin(), step_hits.end(), [](const Hit& x, const Hit& y) {
        return x.a != y.a ? x.a < y.a : x.b < y.b;
    });
    out.insert(out.end(), step_hits.begin(), step_hits.end());
}

}  // namespace kessler
