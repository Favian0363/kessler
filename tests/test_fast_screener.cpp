// screen_grid must give exactly the same hits as screen_bruteforce. Uses real SGP4.

#include "kessler/fast_screener.hpp"
#include "kessler/screener.hpp"
#include "kessler/sgp4_elements.hpp"
#include "kessler/tle.hpp"

#include "hit_compare.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace kessler;

namespace {

const std::string kIss1 = "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927";
const std::string kIss2 = "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537";
const std::string kTrail1 = "1 25544U 98067A   08264.51792528 -.00002182  00000-0 -11606-4 0  2928";
const std::string kFast2 = "2 25544  51.6416 247.4627 0006703 130.5360 324.5288 15.82125391563532";
const std::string kGeo1 = "1 25545U 98067B   08264.26782528  .00000000  00000-0  00000-0 0  9991";
const std::string kGeo2 = "2 25545   0.0500 100.0000 0000100   0.0000  90.0000  1.00270000   100";
const std::string kDecay1 = "1 33333U 05037B   05333.02012661  .25992681  00000-0  24476-3 0  1532";
const std::string kDecay2 = "2 33333  96.4736 157.9986 9950000 244.0492 110.6523  4.00004038 10700";

std::vector<Propagator> build(const std::vector<std::pair<std::string, std::string>>& tles) {
    std::vector<Propagator> objs;
    for (const auto& [l1, l2] : tles) objs.emplace_back(to_sgp4_elements(parse_tle(l1, l2)));
    return objs;
}

}  // namespace

TEST_CASE("screen_grid gives the same hits as screen_bruteforce", "[fast]") {
    const std::vector<std::pair<std::string, std::string>> tles{
        {kIss1, kIss2}, {kIss1, kIss2}, {kTrail1, kIss2}, {kIss1, kFast2}, {kGeo1, kGeo2}};
    // Separate copies, so the two runs can't affect each other.
    auto for_brute = build(tles);
    auto for_grid = build(tles);
    const ScreenConfig cfg{tle_epoch_jd(2008, 264.51782528), 120.0, 10.0, 118.0};

    const auto brute = screen_bruteforce(for_brute, cfg);
    const auto grid = screen_grid(for_grid, cfg);
    REQUIRE(!brute.empty());
    CHECK(first_difference(brute, grid) == -1);
}

TEST_CASE("screen_grid drops a failing object exactly like brute force", "[fast]") {
    const std::vector<std::pair<std::string, std::string>> tles{{kDecay1, kDecay2},
                                                                {kDecay1, kDecay2}};
    auto for_brute = build(tles);
    auto for_grid = build(tles);
    const ScreenConfig cfg{tle_epoch_jd(2005, 333.02012661), 150.0, 300.0, 118.0};

    const auto brute = screen_bruteforce(for_brute, cfg);
    const auto grid = screen_grid(for_grid, cfg);
    REQUIRE(!brute.empty());
    CHECK(first_difference(brute, grid) == -1);
}
