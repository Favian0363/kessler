16 CPU threads
15956 satellites in full catalog

Intro:
Brute force compares every satellite with every other one O(n^2): 3,000 objs = 4.5M pairs per time step

The fast version uses two ideas:
1. Grid (only check neighbors)
- cut the space into cubes (~120km)
    - two objects closer than 120 km must be in the same cube or touching cubes
    - each object only needs to check its own cube and the 26 around it, not all 3000 objects
    - objects are sorted by cube number to enable binary search
2. Threads (parallelization w/ OpenMP)
- propagation
    - each core takes a share of the satellites
- pair search 
    - each core writes hits into its own list

Builds: 
Normal (Slow, Sanitizers)
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
(Before OpenMP: 3.82s) -> (After OpenMP: 0.60s)

Release (Faster, no Sans)
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j --target kessler_screen

## Initial tests (3): [hours, step secs, miss km, objects]

(fraction of catalog 5%-15%)
1. [1 10 5 1000] - serial
2. [6 10 5 3000] - serial
3. [6 10 5 3000] - parallel

1) ./build-release/kessler_screen data/active.tle 1 10 5 1000

objects        1000  (skipped 0 bad records, 0 SGP4 init failures)
window         2026-10-07 10:41:14.682  ->  2026-10-07 11:41:14.682 UTC
sampling       every 10.0 s, 361 samples
thresholds     candidate 118.0 km, miss 5.00 km
pair checks    1.803e+08 in 0.50 s  (3.595e+08 checks/s, includes SGP4)
hits / runs    22438 / 501  (grouping 0.002 s)
conjunctions   12  (refinement 0.012 s)
total          0.52 s
 
TCA (UTC)                object A                  object B                    miss km    rel km/s
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         25575 ISS (UNITY)             0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         26400 ISS (ZVEZDA)            0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         26400 ISS (ZVEZDA)            0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       36086 POISK                   0.000       0.000
2026-10-07 10:42:35.555  31698 TERRASAR-X          36605 TANDEM-X                0.214       0.000
2026-10-07 10:54:10.204  39189 O3B FM4             40348 O3B FM10                3.042       0.002

2) ./build-release/kessler_screen data/active.tle 6 10 5 3000

objects        3000  (skipped 0 bad records, 0 SGP4 init failures)
window         2026-10-07 10:41:14.682  ->  2026-10-07 16:41:14.682 UTC
sampling       every 10.0 s, 2161 samples
thresholds     candidate 118.0 km, miss 5.00 km
pair checks    9.721e+09 in 16.36 s  (5.941e+08 checks/s, includes SGP4)
hits / runs    403315 / 48151  (grouping 0.061 s)
conjunctions   150  (refinement 1.081 s)
total          17.50 s

TCA (UTC)                object A                  object B                    miss km    rel km/s
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         25575 ISS (UNITY)             0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  28358 INTELSAT 10-02      46113 MEV-2                   0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         26400 ISS (ZVEZDA)            0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         26400 ISS (ZVEZDA)            0.000       0.000
2026-10-07 15:26:41.322  39091 BRITE-AUSTRIA       43929 IRIDIUM 171             0.140      14.428
2026-10-07 11:21:19.469  42831 FLYING LAPTOP       49146 STARLINK-3116           0.147       5.205
2026-10-07 10:42:35.555  31698 TERRASAR-X          36605 TANDEM-X                0.214       0.000
2026-10-07 11:09:48.745  46036 STARLINK-1560       48865 COSMOS 2550             0.270      10.332
... and 130 more

#### Compared to parallelized version (5x speedup)

3) ./build-release/kessler_screen data/active.tle 6 10 5 3000 --compare

objects        3000  (skipped 0 bad records, 0 SGP4 init failures)
window         2026-10-07 10:41:14.682  ->  2026-10-07 16:41:14.682 UTC
sampling       every 10.0 s, 2161 samples
thresholds     candidate 118.0 km, miss 5.00 km
brute force    17.55 s  (1 thread)
grid           3.43 s  (16 threads)
speedup        5.1x
identical      YES, all 403315 hits match -> exact answers to brute force
hits / runs    403315 / 48151  (grouping 0.064 s)
conjunctions   150  (refinement 1.088 s)

TCA (UTC)                object A                  object B                    miss km    rel km/s
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         25575 ISS (UNITY)             0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  28358 INTELSAT 10-02      46113 MEV-2                   0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25575 ISS (UNITY)         26400 ISS (ZVEZDA)            0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         26700 ISS (DESTINY)           0.000       0.000
2026-10-07 10:41:24.682  25544 ISS (ZARYA)         26400 ISS (ZVEZDA)            0.000       0.000
2026-10-07 15:26:41.322  39091 BRITE-AUSTRIA       43929 IRIDIUM 171             0.140      14.428
2026-10-07 11:21:19.469  42831 FLYING LAPTOP       49146 STARLINK-3116           0.147       5.205
2026-10-07 10:42:35.555  31698 TERRASAR-X          36605 TANDEM-X                0.214       0.000
2026-10-07 11:09:48.745  46036 STARLINK-1560       48865 COSMOS 2550             0.270      10.332
... and 130 more

### How speed changes with n of threads

grid           7.60 s  (1 threads)

grid           5.45 s  (2 threads)

grid           3.89 s  (4 threads)

grid           3.04 s  (8 threads)

grid           3.26 s  (16 threads)

## First full tests

(full catalog)
1. [24 10 5 15956] - parallel
1. [24 10 5 15956] - parallel + run skipping 

1) linfavian kessler$ ./build-release/kessler_screen data/active.tle 24 10 5

objects        15956  (skipped 0 bad records, 0 SGP4 init failures)
window         2026-10-07 10:41:14.682  ->  2026-10-08 10:41:14.682 UTC
sampling       every 10.0 s, 8641 samples
thresholds     candidate 118.0 km, miss 5.00 km
grid           43.36 s  (16 threads)
hits / runs    47460378 / 15499393  (grouping 9.992 s)
conjunctions   96018  (refinement 352.288 s)

TCA (UTC)                object A                  object B                    miss km    rel km/s
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        53239 CSS (WENTIAN)           0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        67796 CREW DRAGON 12          0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        68689 CYGNUS NG-24            0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        68837 PROGRESS-MS 34          0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       67796 CREW DRAGON 12          0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       68689 CYGNUS NG-24            0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       68837 PROGRESS-MS 34          0.000       0.000
2026-10-07 10:41:24.682  28358 INTELSAT 10-02      46113 MEV-2                   0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               67796 CREW DRAGON 12          0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               68689 CYGNUS NG-24            0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               68837 PROGRESS-MS 34          0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        54216 CSS (MENGTIAN)          0.000       0.000
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        69049 TIANZHOU-10             0.000       0.000
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        69180 SHENZHOU-23 (SZ-23      0.000       0.000
2026-10-07 10:41:24.682  49044 ISS (NAUKA)         67796 CREW DRAGON 12          0.000       0.000
... and 95998 more

96,000 close approaches in 24 hours doesn't sound very realistic, because it isnt.
we're currently not accounting for the fact that many of these "near misses" are actually part of the same fleet.
STARLINK is one of our main culprits with (~8,000) packed together giving us ~50,000 pairs a day just from their fleet.
This isn't a bad finding though, its still a good way to see how packed low orbit really is. 

Also noticed that close to 90% of time spent is on refining. Working on improving that with an earlier suggestion
of skipping impossible runs. 

P.S. Succesful 30x speedup

### Next Changes
- skip runs that can't get within 5km

2) ./build-release/kessler_screen data/active.tle 24 10 5

objects        15956  (skipped 0 bad records, 0 SGP4 init failures)
window         2026-10-07 10:41:14.682  ->  2026-10-08 10:41:14.682 UTC
sampling       every 10.0 s, 8641 samples
thresholds     candidate 118.0 km, miss 5.00 km
grid           43.65 s  (16 threads)
  propagation  10.12 s  (all threads)
  grid setup   14.29 s  (one thread)
  pair search  13.13 s  (all threads)
  merge        6.09 s  (one thread)
hits / runs    47460378 / 15499393  (grouping 9.940 s)
refinement     12.11 s  (refined 5081735 runs, skipped 10417658 that can't get within 5.00 km)
conjunctions   96018

TCA (UTC)                object A                  object B                    miss km    rel km/s
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        53239 CSS (WENTIAN)           0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        67796 CREW DRAGON 12          0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        68689 CYGNUS NG-24            0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        68837 PROGRESS-MS 34          0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       67796 CREW DRAGON 12          0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       68689 CYGNUS NG-24            0.000       0.000
2026-10-07 10:41:24.682  26700 ISS (DESTINY)       68837 PROGRESS-MS 34          0.000       0.000
2026-10-07 10:41:24.682  28358 INTELSAT 10-02      46113 MEV-2                   0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               49044 ISS (NAUKA)             0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               67796 CREW DRAGON 12          0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               68689 CYGNUS NG-24            0.000       0.000
2026-10-07 10:41:24.682  36086 POISK               68837 PROGRESS-MS 34          0.000       0.000
2026-10-07 10:41:24.682  26400 ISS (ZVEZDA)        36086 POISK                   0.000       0.000
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        54216 CSS (MENGTIAN)          0.000       0.000
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        69049 TIANZHOU-10             0.000       0.000
2026-10-07 10:41:24.682  48274 CSS (TIANHE)        69180 SHENZHOU-23 (SZ-23      0.000       0.000
2026-10-07 10:41:24.682  49044 ISS (NAUKA)         67796 CREW DRAGON 12          0.000       0.000
... and 95998 more

