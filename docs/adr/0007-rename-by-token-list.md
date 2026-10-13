# ADR 0007: Rename by replacing a token in listed files

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0005](../rfc/0005-template-registry.md)

## Context

Generated projects kept the template's own project name (`cpp_project_template`,
`qt_qml_simple_hello`). The templates are real, buildable projects maintained on their own.

## Decision

Each template may declare `rename = { token = "...", files = [...] }`. After cloning, dalang
replaces every occurrence of `token` with the project name in those files only. A listed file
that is missing is reported and skipped.

## Alternatives considered

- Placeholders such as `{{project_name}}`: templates would stop building on their own.
- Replace in every file: also rewrites READMEs and docs that talk about the template.
- Rewrite only `project()` with a CMake-aware parser: misses run configs and non-CMake templates.

## Consequences

- Templates stay ordinary projects; the TOML entry carries the knowledge of where the name is.
- When a template moves its name into a new file, its entry must be updated.
- Templates with per-app names (`zephyr`) declare no rename.
