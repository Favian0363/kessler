# 0006: Spatial grid and OpenMP threads for the fast screener

**Status:** accepted

## Context

Brute force checks every pair at every sample: N^2 / 2 checks per step. At
3,000 objects over 6 hours that already took about 16 s, and it grows with N^2.

## Decision

- **Grid.** Space is cut into cubes a hair wider than the candidate threshold.
  A close pair must be in the same or touching cubes, so each object is only
  compared with objects in the 27 cubes around it. Cubes are found by sorting
  objects by a cube number (z in the lowest bits, so 3 cubes stacked in z are
  one search) and binary searching that sorted list. No hash map is built.
- **Threads (OpenMP), per time step.** Propagation is split over objects (each
  object belongs to one thread, so no Propagator is shared). Pair searching is
  split over objects; each thread collects hits separately and they are merged
  and sorted. Time steps stay in order, so "drop an object after its first
  failure" means exactly what it means in brute force.
- **Same answer.** The fast screener must return the same hits as brute force,
  in the same order (distances to 1e-9 km). The tests check this on random
  points, cell edges, far-away clouds and real SGP4 objects, and
  `kessler_screen --compare` checks it on real catalogs.

## Consequences

- Work per step grows roughly with N instead of N^2.
- SGP4 (one call per object per step) becomes the main cost; interpolating
  between coarser SGP4 samples is the next optimization if needed.
