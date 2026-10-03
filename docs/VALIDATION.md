# Validation

Record the exact commands, data dates, and results so anyone can reproduce them.

## 1. TLE parsing and SGP4 propagation

**Unit tests** (`tests/test_tle.cpp`, `test_time.cpp`, `test_sgp4_elements.cpp`):
parser fields checked column by column; Julian dates checked against an
independent algorithm (Fliegel-Van Flandern); unit conversions checked against
values computed separately in Python; regression tests for every parser failure
mode found in review.

**Differential test** (`tests/verify_vallado.cpp`): for every record in Vallado's
`SGP4-VER.TLE`, at every time from the record's start/stop/step columns, kessler's
pipeline (own parser -> unit conversion -> `sgp4init`) is compared with Vallado's
pipeline (`twoline2rv` -> `sgp4`). Required: identical epochs (1e-9 days),
identical error codes at identical times, positions within 1 mm, velocities
within 1 nm/s. Run in both improved and afspc modes.

| Mode | Records | Samples | Worst position diff | Worst velocity diff | Result |
|---|---|---|---|---|---|
| improved | x | 656 | 1.774e-10 km | 6.128e-14 km/s | TBD |

656 samples compared. worst |dr| = 1.774e-10 km, worst |dv| = 6.128e-14 km/s, worst epoch diff = 0.000e+00 days

## 2. Optimized screeners vs. brute-force oracle
## 3. Full catalog vs. CelesTrak SOCRATES
