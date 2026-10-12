# Changelog

All notable changes to dalang. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/).

## [2.0.0] - 2026-10-13

Renamed from cecep to dalang. The command line changed, so this is a major release.

### Added
- `dalang new <dest>` with `-t/--template`, `-n/--name`, `-r/--ref` (branch or tag),
  `--no-git-init` and `-i/--interactive` (pick template, destination and name in a form).
- `dalang list` shows the templates and where user templates are read from.
- Templates are defined in `templates.toml`, embedded in the binary. Users can add or override
  templates in `%APPDATA%\dalang\templates.toml` or `~/.config/dalang/templates.toml`, or pass a
  file with `-c/--config`.
- Project rename: each template can list the files where its own project name is replaced by
  the new one (`rename = { token, files }`).
- The generated project gets a short README naming its template.
- `-v/--verbose` prints each git command and its output, `-q/--quiet` prints errors only,
  `-V/--version` prints the version.
- Release workflow publishing Windows, Linux and macOS binaries on `v*` tags.

### Changed
- Positional `dest type` arguments replaced by the `new` subcommand and `-t`.
- Clones are shallow (`--depth 1`), and git never prompts for credentials.
- git errors include git's own output.
- Process handling moved from cpp-subprocess to reproc, which is available on both Conan and
  vcpkg.

### Fixed
- `--help` and invalid arguments no longer end in an uncaught exception.
- An existing, non-empty destination is refused before anything runs.
- A failed generation removes what it created.
- Non-ASCII paths work on Windows.

## [1.1.0] - 2026-10-12

### Added
- `qml` template (Qt 6 / QML desktop app).

## [1.0.0] - 2026-09-28

First version, as cecep.

### Added
- Generates a project by cloning a template, deleting its `.git` and README, and running
  `git init`.
- Templates: `full`, `min`, `zephyr`, `idf`.

[2.0.0]: https://github.com/royyandzakiy/dalang/releases/tag/v2.0.0
