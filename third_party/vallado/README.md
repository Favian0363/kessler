# Vallado reference SGP4

1. Go to https://celestrak.org/publications/AIAA/2006-6753/
2. Download the C++ source and copy `SGP4.cpp` and `SGP4.h` into this folder.
   (If the current release names files differently, adjust the path in the
   top-level CMakeLists.txt.)
3. Copy the verification input `SGP4-VER.TLE` into `tests/data/`. The
   differential test (`tests/verify_vallado.cpp`) compares kessler against
   Vallado's own pipeline, so no separate expected-output file is needed.

Per the authors' FAQ, the code carries no license, and users are asked to cite
the paper and link to the page above in documentation and source. This
repository does so in the top-level README.
