# ADR 0005: std::expected for errors; C++23 limited to what GCC 13 supports; fmt for output

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0004](../rfc/0004-core-restructure.md)

## Context

Most failures in dalang are ordinary (git missing, folder exists, bad TOML) and must reach the
user as one clear message. The project is C++23, but Linux CI builds with Clang 21 against
Ubuntu 24.04's libstdc++ 13, and with GCC 13.

## Decision

- Every fallible function returns `dalang::Result<T>` = `std::expected<T, Error>`; steps are
  chained with `and_then`, `or_else`, `transform` and `transform_error`. Exceptions are only
  caught at the CLI boundary (CLI11, unexpected `std::exception`).
- Use C++23 features that libstdc++ 13 and current libc++ have: `std::expected` and its monadic
  operations, concepts, designated initializers, `starts_with` / `contains`, lambdas without
  `()` before `->`.
- Not yet: `std::print`, `std::ranges::to`, `vector::append_range` (libstdc++ 14),
  `views::enumerate` (libc++ 19). Formatting and printing stay on **fmt**.

## Alternatives considered

- Exceptions: failures would not show in signatures, and the happy path is not hot.
- `std::error_code`: dalang's errors are messages for a person, not codes for a program.
- Install GCC 14 in CI to allow `std::print`: changes the toolchain the templates share.

## Consequences

- Switching to `std::print` / `std::format` later is a mechanical change once CI moves to GCC 14.
- `Error` is just a message; add a code only if a caller ever needs to branch on it.
