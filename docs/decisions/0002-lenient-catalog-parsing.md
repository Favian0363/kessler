# 0002: Lenient catalog parsing, strict record parsing

**Status:** accepted

## Context

Real catalogs contain tens of thousands of records. A single malformed record
(truncated line, bad checksum, orphaned line) is plausible in any download.

## Decision

`parse_tle()` is strict: any defect throws, naming the field and columns.
`parse_tle_stream()` is strict too (first problem throws, with its line number)
and is used in tests. `parse_tle_stream_lenient()` skips bad records and
returns them as "line N: reason" messages; the CLI uses it and reports every
skipped record.

## Consequences

- One corrupt record cannot abort a full-catalog run.
- Skipped records are always visible, never silent; a run's output states how
  many objects were excluded and why.
