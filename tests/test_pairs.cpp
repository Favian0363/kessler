// Tests for the brute-force pair finder. These use made-up points, so every
// expected distance can be worked out by hand. No SGP4 involved.

#include "kessler/pairs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <stdexcept>
#include <vector>

using Catch::Matchers::WithinAbs;
using namespace kessler;

namespace {
std::vector<char> all_alive(std::size_t n) { return std::vector<char>(n, 1); }
}  // namespace

TEST_CASE("two objects closer than the threshold give one hit", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {3, 4, 0}};  // 5 km apart (a 3-4-5 triangle)
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, all_alive(2), 6.0, 7, hits);

    REQUIRE(hits.size() == std::size_t{1});
    CHECK(hits[0].a == 0u);
    CHECK(hits[0].b == 1u);
    CHECK(hits[0].step == 7);
    CHECK_THAT(hits[0].distance_km, WithinAbs(5.0, 1e-12));
}

TEST_CASE("the threshold is strict: exactly on the threshold is not a hit", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {3, 4, 0}};  // exactly 5 km apart
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, all_alive(2), 5.0, 0, hits);
    CHECK(hits.empty());
    find_close_pairs_bruteforce(pos, all_alive(2), 4.9, 0, hits);
    CHECK(hits.empty());
}

TEST_CASE("all three coordinates count", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {1, 2, 2}};  // sqrt(1 + 4 + 4) = 3 km
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, all_alive(2), 3.5, 0, hits);
    REQUIRE(hits.size() == std::size_t{1});
    CHECK_THAT(hits[0].distance_km, WithinAbs(3.0, 1e-12));
}

TEST_CASE("only the close pair is reported", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {1, 0, 0}, {100, 0, 0}};
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, all_alive(3), 10.0, 0, hits);
    REQUIRE(hits.size() == std::size_t{1});
    CHECK(hits[0].a == 0u);
    CHECK(hits[0].b == 1u);
    CHECK_THAT(hits[0].distance_km, WithinAbs(1.0, 1e-12));
}

TEST_CASE("every pair is checked exactly once, in order", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {0.1, 0, 0}, {0.2, 0, 0}, {0.3, 0, 0}};
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, all_alive(4), 10.0, 0, hits);

    // 4 objects -> 6 pairs, and a < b, ordered by a and then b.
    const std::uint32_t expected[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    REQUIRE(hits.size() == std::size_t{6});
    for (std::size_t k = 0; k < 6; ++k) {
        CHECK(hits[k].a == expected[k][0]);
        CHECK(hits[k].b == expected[k][1]);
    }
}

TEST_CASE("objects that are not alive are skipped", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    const std::vector<char> alive{1, 0, 1};  // the middle object is dead
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, alive, 10.0, 0, hits);
    REQUIRE(hits.size() == std::size_t{1});
    CHECK(hits[0].a == 0u);
    CHECK(hits[0].b == 2u);
}

TEST_CASE("hits are appended across calls, with their own step number", "[pairs]") {
    const std::vector<Vec3> pos{{0, 0, 0}, {1, 0, 0}};
    std::vector<Hit> hits;
    find_close_pairs_bruteforce(pos, all_alive(2), 10.0, 0, hits);
    find_close_pairs_bruteforce(pos, all_alive(2), 10.0, 1, hits);
    REQUIRE(hits.size() == std::size_t{2});
    CHECK(hits[0].step == 0);
    CHECK(hits[1].step == 1);
}

TEST_CASE("zero or one object gives no hits and no error", "[pairs]") {
    std::vector<Hit> hits;
    find_close_pairs_bruteforce({}, {}, 10.0, 0, hits);
    find_close_pairs_bruteforce({{0, 0, 0}}, all_alive(1), 10.0, 0, hits);
    CHECK(hits.empty());
}

TEST_CASE("bad input is rejected", "[pairs]") {
    std::vector<Hit> hits;
    const std::vector<Vec3> pos{{0, 0, 0}, {1, 0, 0}};
    CHECK_THROWS_AS(find_close_pairs_bruteforce(pos, all_alive(3), 10.0, 0, hits),
                    std::invalid_argument);  // sizes differ
    CHECK_THROWS_AS(find_close_pairs_bruteforce(pos, all_alive(2), 0.0, 0, hits),
                    std::invalid_argument);
    CHECK_THROWS_AS(find_close_pairs_bruteforce(pos, all_alive(2), -5.0, 0, hits),
                    std::invalid_argument);
}
