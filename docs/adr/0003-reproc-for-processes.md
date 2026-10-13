# ADR 0003: Use reproc for child processes

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0003](../rfc/0003-honest-build.md)

## Context

cpp-subprocess has no vcpkg port, so the vcpkg build could not work, and its `Popen` + `wait()`
pattern can deadlock when a child fills a pipe.

## Decision

Use **reproc++** (`reproc/14.2.5`): both Conan and vcpkg provide the `reproc++` CMake target.
`ReprocRunner` pipes stdout and stderr, reads both with `reproc::drain` into one string, discards
stdin and extends the environment with the command's variables.

## Alternatives considered

- FetchContent cpp-subprocess for vcpkg only: two dependency paths for one library.
- Boost.Process: Boost in a small CLI.
- Own CreateProcess / posix_spawn code: more to maintain on three OSes.

## Consequences

- One dependency name on both package managers.
- reproc converts arguments to UTF-16 on Windows, so non-ASCII paths reach git intact.
- All process use goes through `ReprocRunner`; tests never need reproc (ADR 0004).
