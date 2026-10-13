# RFCs

Design write-ups for significant changes and findings: what was wrong or needed (with evidence),
the architecture before and after, why this approach over the alternatives, and how it was
implemented and verified. Decisions that shape the architecture also get a short record in
[`../adr/`](../adr/README.md).

One chronological series. The **Origin** column says where each document comes from:

- **history**: how cecep worked before the overhaul, written from its code and git history.
- **overhaul**: the October 2026 rename, restructure and release work, with commits.

Older documents describe the code as it was then; where today's code differs, they say so
("Outcome").

| # | Title | Origin | Status | Date | ADR |
|---|---|---|---|---|---|
| [0001](0001-cecep-first-generator.md) | cecep, the first generator | history | Historical (describes cecep v1.x) | 2026-06-20 – 2026-10-12 | [0001](../adr/0001-git-cli-as-child-process.md) |
| [0002](0002-rename-to-dalang.md) | Rename cecep to dalang | overhaul | Implemented | 2026-10-13 | [0002](../adr/0002-name-dalang.md) |
| [0003](0003-honest-build.md) | An honest build: template leftovers, reproc, clang-tidy, coverage | overhaul | Implemented | 2026-10-13 | [0003](../adr/0003-reproc-for-processes.md) |
| [0004](0004-core-restructure.md) | Core restructure: duck-typed runner, std::expected, three files | overhaul | Implemented | 2026-10-13 | [0004](../adr/0004-concepts-over-interfaces.md), [0005](../adr/0005-expected-errors-and-fmt.md) |
| [0005](0005-template-registry.md) | Template registry in TOML, with project rename | overhaul | Implemented | 2026-10-13 | [0006](../adr/0006-embedded-templates-toml.md), [0007](../adr/0007-rename-by-token-list.md) |
| [0006](0006-command-line-v2.md) | Command line v2 and safe generation | overhaul | Implemented | 2026-10-13 | [0008](../adr/0008-subcommand-cli.md) |
| [0007](0007-releases-and-docs.md) | Release binaries and project documentation | overhaul | Implemented (workflow untested until first tag) | 2026-10-13 | [0009](../adr/0009-tag-driven-releases.md) |
| [0008](0008-ci-green.md) | Getting CI green on all 17 jobs | overhaul | Implemented | 2026-10-13 | – |

## Format

Header (Status, Date, Commits, ADR, Related), then **Summary**, **Findings** (with evidence),
**Architecture**, **Alternatives**, **Implementation**, **Verification** / **Outcome**,
**Follow-ups**. Sections without content are left out.

## Writing a new one

Copy the newest RFC, take the next number, and add a row above. Write one when a change alters
behaviour users see, the structure of the code or the build, or needs evidence to justify; small
fixes only need a CHANGELOG line.
