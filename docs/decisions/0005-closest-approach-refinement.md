# 0005: How hits become close approaches

**Status:** accepted

## Context

The screener only says "these two were within the candidate threshold at
this sample." The real question is how close they actually got, and when.

## Decision

1. **Candidate threshold** = miss distance + 22.4 km/s x half a step + 1 km.
   22.4 km/s is two objects at Earth's escape speed head-on, an upper bound for
   any two Earth-orbiting objects. This guarantees no pass within the miss
   distance can hide between two samples. (A tighter LEO-only bound, ~15 km/s,
   is a possible optimization later, but is not safe for highly elliptical orbits.)
2. **Runs.** Each pair's hits are split into runs of consecutive steps. One run
   is one encounter.
3. **Refinement.** For each run, the exact closest moment is searched between the
   sample before and the sample after the run's closest sample, with
   golden-section search on the SGP4 distance, to 60 microseconds.
4. **Report** the encounter if the refined distance is below the miss distance,
   with time of closest approach, miss distance and relative speed.

## Consequences

- An encounter already under way at the start (or still under way at the end)
  is reported at the window edge, not at its true closest moment.
- Objects flying together (docked, or just released from the same rocket) form
  one long run and show up as a near-zero miss at near-zero relative speed.
  They are reported, not filtered; the relative speed makes them easy to spot.
