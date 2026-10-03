# kessler

**satellite conjunction screening engine in C++20.**

kessler predicts close approaches between every publicly tracked object in Earth
orbit over a 7-day window and checks its answers against CelesTrak's SOCRATES
conjunction reports.

<!-- want to replace with a GIF of the Cesium globe visualization -->

![CI](https://github.com/Favian0363/kessler/actions/workflows/ci.yml/badge.svg)

## Results

| Metric | Value |
|---|---|
| Objects screened | TBD |
| Screening window | 7 days |
| Naive all-pairs runtime | TBD |
| kessler runtime | TBD |
| Speedup | TBD |
| SOCRATES recall / precision | TBD |
| Hardware | TBD |

## Hard part

There are over 30,000 tracked objects, which means hundreds of millions of
pairs to check at every time step, and objects close on each other at up to
15 kms. A brute force search takes hours; a naive "fast" search silently
misses collisions that happen between time steps. kessler is fast and
provably agrees with a brute-force oracle. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/kessler data/catalog.tle
```

See [data/README.md](data/README.md) for downloading orbit data.

## How correctness is verified

1. SGP4 propagation matches the official Vallado test vectors.
2. Every optimized screener must produce **identical** results to a brute-force
   oracle on a reference subset.
3. Full-catalog results are compared against CelesTrak SOCRATES.

Details: [docs/VALIDATION.md](docs/VALIDATION.md).

## Roadmap

- [x] Phase 0: build system, tests, CI
- [ ] Phase 1: TLE parsing + SGP4 propagation (verified against Vallado test vectors)
- [ ] Phase 2: brute-force screening oracle
- [ ] Phase 3: time-of-closest-approach refinement
- [ ] Phase 4: spatial grid + orbital filters (verified identical to oracle)
- [ ] Phase 5: SoA layout, interpolation, OpenMP, AVX2, benchmarks
- [ ] Phase 6: full-catalog validation against SOCRATES
- [ ] Phase 7: 3D visualization (CesiumJS / CZML)

## Acknowledgments

- SGP4 reference implementation: Vallado, D. A., Crawford, P., Hujsak, R., and
  Kelso, T. S., "Revisiting Spacetrack Report #3," AIAA 2006-6753.
  https://celestrak.org/publications/AIAA/2006-6753/
- Orbital data and SOCRATES reports: [CelesTrak](https://celestrak.org).
