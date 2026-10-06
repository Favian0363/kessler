# 0004: The brute-force screener is the answer key

**Status:** accepted

## Context

The fast screener (spatial grid, filters, threads, vectorization) is easy to
get subtly wrong: a pair that is missed is never reported, so nothing looks
broken. We need a trusted result to compare it with.

## Decision

Write the slowest, simplest version first: at every time sample, move every
object to that time with SGP4 and check every pair. Every faster version must
return exactly the same list of hits, in the same order, on the same input.

Rules that define "the same":

- A hit is a pair closer than the threshold at one time sample. A pair exactly
  on the threshold is NOT a hit (strict less-than).
- Samples are at k * step for k = 0 .. floor(duration / step), so the last
  sample lands on the end time.
- Hits are ordered by step, then a, then b, with a < b.
- a and b are positions in the input list, not catalog numbers.
- An object that fails to propagate is dropped for the rest of the run.
- Each object's offset from its own epoch to the start time is worked out once,
  not at every step.

## Consequences

- Fast versions can be tested by comparing their hit lists with this one.
- This phase only finds "close at a sample". Turning hits into real close
  approaches (finding the exact closest moment) is a later phase.
- Too slow for the full catalog by design: 30,000 objects over 7 days at 10 s
  steps is about 2.7e13 distance checks.
