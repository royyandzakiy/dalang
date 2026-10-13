# ADR 0008: Subcommand CLI that refuses or undoes instead of overwriting

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0006](../rfc/0006-command-line-v2.md)

## Context

cecep took `dest type` positionally, crashed on `--help`, lost git's errors and left half-created
folders behind.

## Decision

- Subcommands: `dalang new <destination> [options]` and `dalang list`; global `-v`, `-q`, `-c`,
  `-V`.
- Validate name, destination and git before running anything; refuse a non-empty destination
  (no `--force`).
- Shallow clone with `GIT_TERMINAL_PROMPT=0`; on any later failure, remove what dalang created.
- Breaking change, so the version is 2.0.0.

## Alternatives considered

- Keep positional arguments (no room for `list` and new options); `--force` (deletes data on a
  typo); continue on failure and report (leaves broken projects).

## Consequences

- Scripts written for cecep need `new` and `-t`.
- A failed run leaves the destination as it was, except that an existing empty folder stays.
