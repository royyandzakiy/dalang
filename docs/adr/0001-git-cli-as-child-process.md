# ADR 0001: Drive the git CLI as a child process

- **Status:** Accepted
- **Date:** 2026-07-01 (`142ec6b`), kept in 2026-10-13
- **RFC:** [0001](../rfc/0001-cecep-first-generator.md), [0003](../rfc/0003-honest-build.md)

## Context

dalang needs three git operations: check that git exists, clone (shallow, optionally at a tag or
branch) and init. Users already have git installed, configured with their credentials, proxies
and SSH keys.

## Decision

Run the `git` executable as a child process for every git operation, with arguments built in
`git::clone` / `git::init` / `git::check_available` and output captured for error messages.

## Alternatives considered

- **libgit2:** a large native dependency, its own credential and SSH handling, and shallow clone
  support that lags the CLI.
- **Download a tarball from the host's archive API:** GitHub-only, and no `--ref` for arbitrary
  hosts.

## Consequences

- git must be on `PATH`; dalang checks this first and says so.
- Whatever works in the user's terminal (SSH URLs, credential helpers, proxies) works in dalang.
- Behaviour depends on the installed git version; dalang only uses long-stable flags
  (`--depth`, `--branch`, `-C`).
