// The grid must give exactly the same hits as brute force. No SGP4 here:
// random points, points on cell edges, dead objects, far-away clouds.

#include "kessler/grid.hpp"
#include "kessler/pairs.hpp"

#include "hit_compare.hpp"

#include <catch2/catch_test_macros.hpp>

#include <random>
#include <stdexcept>
#include <vector>

using namespace kessler;

namespace {

std::vector<Vec3> random_points(std::size_t n, double half_width, Vec3 center, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> u(-half_width, half_width);
    std::vector<Vec3> pts(n);
    for (auto& p : pts) p = {center.x + u(rng), center.y + u(rng), center.z + u(rng)};
    return pts;
}

std::vector<char> random_alive(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<char> alive(n);
    for (auto& a : alive) a = (rng() % 10 == 0) ? 0 : 1;  // about 10% dead
    return alive;
}

}  // namespace

TEST_CASE("grid matches brute force on random points", "[grid]") {
    for (unsigned seed = 1; seed <= 5; ++seed) {
        for (const double threshold : {10.0, 80.0, 250.0}) {
            // Box size scales with the threshold so every case has plenty of hits.
            const auto pts = random_points(500, 10.0 * threshold, {0, 0, 0}, seed);
            const auto alive = random_alive(pts.size(), seed + 100);
            std::vector<Hit> brute, grid;
            find_close_pairs_bruteforce(pts, alive, threshold, 3, brute);
            find_close_pairs_grid(pts, alive, threshold, 3, grid);
            CHECK(brute.size() > std::size_t{20});  // the test is only meaningful with hits
            CHECK(first_difference(brute, grid) == -1);
        }
    }
}

TEST_CASE("grid matches brute force far from the origin", "[grid]") {
    // A cloud out at geostationary distance, with negative coordinates.
    const auto pts = random_points(400, 800.0, {42000.0, -42000.0, -500.0}, 7);
    const std::vector<char> alive(pts.size(), 1);
    std::vector<Hit> brute, grid;
    find_close_pairs_bruteforce(pts, alive, 118.0, 0, brute);
    find_close_pairs_grid(pts, alive, 118.0, 0, grid);
    CHECK(brute.size() > std::size_t{20});
    CHECK(first_difference(brute, grid) == -1);
}

TEST_CASE("grid matches brute force for points on cell edges", "[grid]") {
    // A lattice: spaced exactly one threshold apart, then a hair closer.
    for (const double spacing : {100.0, 99.999}) {
        std::vector<Vec3> pts;
        for (int x = -3; x <= 3; ++x)
            for (int y = -3; y <= 3; ++y)
                for (int z = -3; z <= 3; ++z) pts.push_back({x * spacing, y * spacing, z * spacing});
        const std::vector<char> alive(pts.size(), 1);
        std::vector<Hit> brute, grid;
        find_close_pairs_bruteforce(pts, alive, 100.0, 0, brute);
        find_close_pairs_grid(pts, alive, 100.0, 0, grid);
        CHECK(first_difference(brute, grid) == -1);
        if (spacing == 100.0) CHECK(brute.empty());   // exactly on the threshold: not a hit
        if (spacing < 100.0) CHECK(!brute.empty());   // neighbors along each axis are hits
    }
}

TEST_CASE("grid keeps appending across steps like brute force", "[grid]") {
    const auto pts = random_points(300, 600.0, {0, 0, 0}, 11);
    const std::vector<char> alive(pts.size(), 1);
    std::vector<Hit> brute, grid;
    for (int step = 0; step < 3; ++step) {
        find_close_pairs_bruteforce(pts, alive, 60.0, step, brute);
        find_close_pairs_grid(pts, alive, 60.0, step, grid);
    }
    CHECK(first_difference(brute, grid) == -1);
}

TEST_CASE("grid rejects bad input", "[grid]") {
    std::vector<Hit> hits;
    const std::vector<Vec3> pts{{0, 0, 0}, {1, 0, 0}};
    CHECK_THROWS_AS(find_close_pairs_grid(pts, std::vector<char>(3, 1), 10.0, 0, hits),
                    std::invalid_argument);
    CHECK_THROWS_AS(find_close_pairs_grid(pts, std::vector<char>(2, 1), 0.0, 0, hits),
                    std::invalid_argument);
    // A 1-millimeter cell with a point 1000 km out is about a billion cells away.
    const std::vector<Vec3> far{{0, 0, 0}, {1000.0, 0, 0}};
    CHECK_THROWS_AS(find_close_pairs_grid(far, std::vector<char>(2, 1), 1e-6, 0, hits),
                    std::out_of_range);
}
