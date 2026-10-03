# 0001: Use Vallado's reference SGP4 as the correctness oracle

**Status:** accepted

## Context

SGP4 is ~1,000+ lines of dense numerical code, including a deep-space branch
(SDP4) for orbits with periods of 225 minutes or more. Writing it from scratch
first would delay everything else and risk subtle errors.

## Decision

Phase 1 uses Vallado et al.'s published reference implementation unmodified
and verifies it against the official test vectors. In Phase 5, the hot path
(near-Earth objects, the large majority of the catalog) gets a vectorized
structure-of-arrays implementation, which must match the reference to within a
documented tolerance. Deep-space objects keep using the scalar reference.

## Consequences

- Correct propagation from day one; optimization work is always checked
  against a trusted baseline.
- Two SGP4 code paths to maintain; mitigated by a test comparing them on the
  full catalog.
