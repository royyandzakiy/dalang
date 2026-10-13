# Architecture Decision Records

One short record per architectural decision: the context, what was decided, what was rejected,
and what follows from it. The reasoning and evidence are in the matching
[RFC](../rfc/README.md). Bug fixes and investigations have RFCs but no ADR.

Same Origin column as the RFCs.

| # | Title | Origin | Status | Date | RFC |
|---|---|---|---|---|---|
| [0001](0001-git-cli-as-child-process.md) | Drive the git CLI as a child process | history | Accepted | 2026-07-01 | [0001](../rfc/0001-cecep-first-generator.md), [0003](../rfc/0003-honest-build.md) |
| [0002](0002-name-dalang.md) | Name the tool dalang | overhaul | Accepted | 2026-10-13 | [0002](../rfc/0002-rename-to-dalang.md) |
| [0003](0003-reproc-for-processes.md) | Use reproc for child processes | overhaul | Accepted | 2026-10-13 | [0003](../rfc/0003-honest-build.md) |
| [0004](0004-concepts-over-interfaces.md) | Concepts and templates instead of interfaces | overhaul | Accepted | 2026-10-13 | [0004](../rfc/0004-core-restructure.md) |
| [0005](0005-expected-errors-and-fmt.md) | std::expected for errors; C++23 limited to what GCC 13 supports; fmt for output | overhaul | Accepted | 2026-10-13 | [0004](../rfc/0004-core-restructure.md) |
| [0006](0006-embedded-templates-toml.md) | Templates in an embedded templates.toml, merged with a user file | overhaul | Accepted | 2026-10-13 | [0005](../rfc/0005-template-registry.md) |
| [0007](0007-rename-by-token-list.md) | Rename by replacing a token in listed files | overhaul | Accepted | 2026-10-13 | [0005](../rfc/0005-template-registry.md) |
| [0008](0008-subcommand-cli.md) | Subcommand CLI that refuses or undoes instead of overwriting | overhaul | Accepted | 2026-10-13 | [0006](../rfc/0006-command-line-v2.md) |
| [0009](0009-tag-driven-releases.md) | Releases are built by GitHub Actions from a version tag | overhaul | Accepted | 2026-10-13 | [0007](../rfc/0007-releases-and-docs.md) |

## Format

**Status** (Proposed / Accepted / On hold / Superseded by NNNN), **Date**, **RFC**, then
**Context**, **Decision**, **Alternatives considered**, **Consequences**.

A decision is never edited away: when it changes, add a new ADR and set the old one's status to
"Superseded by NNNN".
