# ADR 0006: Templates in an embedded templates.toml, merged with a user file

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0005](../rfc/0005-template-registry.md)

## Context

Templates were a `constexpr` array in `main.cpp`; adding one meant a rebuild, and users could
not add their own.

## Decision

- Built-in templates live in `templates.toml` at the repository root, as `[[template]]` entries,
  embedded into the binary at configure time.
- At run time they are merged with `--config FILE` or, if absent, the user file
  (`%APPDATA%\dalang\templates.toml`, `$XDG_CONFIG_HOME/dalang/...`, `~/.config/dalang/...`).
  Same name replaces in place; new names are appended.

## Alternatives considered

- JSON (no comments); a file next to the executable (breaks for installed binaries); user file
  replacing the built-ins (users would copy them all).

## Consequences

- The binary works with no files beside it.
- Adding a built-in template is a TOML edit plus the test that checks the list.
- A broken user file is an error with file and line, not a silent fallback.
