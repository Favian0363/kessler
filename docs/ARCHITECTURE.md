# Architecture

<!-- Grow this document phase by phase. Keep a diagram at the top (Mermaid
     renders natively on GitHub). -->

```mermaid
flowchart LR
    A[TLE catalog] --> B[Parse]
    B --> C[SGP4 propagation]
    C --> D[Candidate screening]
    D --> E[Closest-approach refinement]
    E --> F[Conjunction report]
    F --> G[Validation vs SOCRATES]
    F --> H[CZML visualization]
```

## Coordinate frame

SGP4 outputs position and velocity in the TEME frame. Every object is in the
same frame, and the distance between two points does not change when the frame
rotates, so conjunction screening needs no frame conversion. Conversion to
Earth-fixed coordinates is only needed for visualization.

## Time

Each TLE has its own epoch. To compare objects, every object is propagated to
the same absolute UTC instants: `minutes_since_epoch_i = (t - epoch_i)`.

## Known limitations

- **Leap seconds.** Julian Date arithmetic on UTC assumes 86,400-second days.
  If a leap second fell inside a screening window, elapsed time would be off by
  1 s (~7.7 km along-track for LEO). None has been inserted since the end of 2016.
- **Catalog numbers above 99,999** (Alpha-5 format) are not yet supported.

## Components

_TBD as phases are completed._
