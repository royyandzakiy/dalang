# Changelog

Every item links to the RFC with the evidence and reasoning; RFCs and ADRs are indexed in
[docs/rfc](docs/rfc/README.md) and [docs/adr](docs/adr/README.md). Versions follow
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Bug Fix

- CI failed on 12 of 17 jobs: sanitizer trip-wires built without sanitizers, clang-cl Conan profiles pinned to VS 2022, MSVC ASan `annotate_optional` mismatch on VS 2026, MinGW tests loading a foreign `libstdc++-6.dll`, real-git tests assuming LF line endings ([RFC 0008](docs/rfc/0008-ci-green.md))

## [v2.0.0]

**Release Date:** 2026-10-13

Renamed from cecep to dalang, with a new command line, so this is a major release.

### Feature

- Renamed to dalang ([RFC 0002](docs/rfc/0002-rename-to-dalang.md))
- `dalang new <dest>` with `-t/--template`, `-n/--name`, `-r/--ref` (branch or tag), `--no-git-init` and `-i/--interactive`; `dalang list` ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- Templates defined in `templates.toml`, embedded in the binary; add or override them in `%APPDATA%\dalang\templates.toml` or `~/.config/dalang/templates.toml`, or with `-c/--config` ([RFC 0005](docs/rfc/0005-template-registry.md))
- The template's own project name is replaced with the new one in the files each template lists ([RFC 0005](docs/rfc/0005-template-registry.md))
- The generated project gets a short README naming its template ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- `-v/--verbose` prints each git command and its output, `-q/--quiet` prints errors only, `-V/--version` ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- Release archives for Windows, Linux and macOS, built from `v*` tags ([RFC 0007](docs/rfc/0007-releases-and-docs.md))

### Bug Fix

- `--help` and invalid arguments ended in an uncaught exception ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- git's error output was discarded; errors now include it ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- A non-empty destination was not refused, and a failed run left a half-created folder; dalang now checks first and removes what it created ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- git could wait forever for credentials on a private or mistyped URL ([RFC 0006](docs/rfc/0006-command-line-v2.md))
- Non-ASCII destination paths were mangled on Windows ([RFC 0004](docs/rfc/0004-core-restructure.md))
- The test build referenced sources that no longer existed, and the vcpkg build could not resolve cpp-subprocess ([RFC 0003](docs/rfc/0003-honest-build.md))

### Development

- Core split into `core.hpp/.cpp` and a header-only `generator.hpp`; git runs through a runner checked by `ProcessRunnerConcept`, faked in tests without interfaces; errors are `std::expected` ([RFC 0004](docs/rfc/0004-core-restructure.md))
- reproc replaces cpp-subprocess, available on both Conan and vcpkg ([RFC 0003](docs/rfc/0003-honest-build.md))
- Template leftovers (MyLib, greeter, lib-only presets, version header) removed; clang-tidy fixed on clang-cl; coverage re-enabled in CI ([RFC 0003](docs/rfc/0003-honest-build.md))
- 32 tests, including real git against a temporary local repository ([RFC 0004](docs/rfc/0004-core-restructure.md))
- README, LICENSE (MIT), CONTRIBUTING; RFCs, ADRs and architecture stages in `docs/` ([RFC 0007](docs/rfc/0007-releases-and-docs.md))

## [v1.1.0]

**Release Date:** never tagged; cecep, 2026-06-20 to 2026-10-12. The version is what
`version.txt` said at the time. See [RFC 0001](docs/rfc/0001-cecep-first-generator.md).

### Feature

- `qml` template (2026-10-12)
- `idf` template (2026-09-30)
- `zephyr` template (2026-09-28)
- Generate a project by cloning a template, deleting its `.git` and README, and running `git init`; `full` and `min` templates (2026-07-01)
