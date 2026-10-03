# 0003: WGS-72 constants, SGP4 "improved" operation mode

**Status:** accepted

## Context

SGP4 can run with WGS-72 or WGS-84 gravity constants, and in "afspc" mode
(reproduces the original Air Force code) or "improved" mode (Vallado's fixes).

## Decision

Use WGS-72, because TLEs are generated with WGS-72 and other constants give
slightly wrong results. Use improved mode, matching Vallado's recommendation
and python-sgp4's default. The differential verification runs in both modes.

## Consequences

- Results match standard tooling.
- If a comparison source (e.g. an operational product) uses afspc mode,
  small differences in deep-space cases are expected and explainable.
