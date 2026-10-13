# RFC 0005: Template registry in TOML, with project rename

- **Status:** Implemented
- **Date:** 2026-10-13
- **Commits:** `a770d10`, `1d4ec2b`
- **ADR:** [0006](../adr/0006-embedded-templates-toml.md), [0007](../adr/0007-rename-by-token-list.md)

## Summary

Move the template list out of the source into `templates.toml`, let users add their own without
rebuilding, and rename the generated project instead of leaving the template's name in it.

## Findings

- Adding a template meant editing a `constexpr std::array` and its size, then rebuilding
  (`4eb7600` "updating array count"). Users could not add their own.
- Generated projects kept the template's project name: `project(cpp_project_template)` in
  `CMakeLists.txt`, `qt_qml_simple_hello` in the QML one. DEVELOPMENT.md already planned "separate
  mapping into .json/.toml" and "toml to provide substitutions".
- Where each template's name lives (from shallow clones on 2026-10-13):

  | Template | Token | Files |
  |---|---|---|
  | `min` | `cpp_project_template` | `CMakeLists.txt` |
  | `full` | `cpp_project_template` | `CMakeLists.txt`, `.run/app.run.xml`, `.run/unit_tests.run.xml` |
  | `qml` | `qt_qml_simple_hello` | `CMakeLists.txt` |
  | `idf` | `esp-idf-dynamic-freq-scaling` | `CMakeLists.txt` |
  | `zephyr` | none at the root; `project(hello_world)` per app under `apps/` | – |

## Architecture

```toml
[[template]]
name = "min"
url = "https://github.com/royyandzakiy/cpp-project-template-min"
description = "Minimal C++23 CMake project with presets, Conan/vcpkg and GoogleTest"
rename = { token = "cpp_project_template", files = ["CMakeLists.txt"] }
```

- `templates.toml` at the repository root is read at configure time and embedded as a raw string
  in the generated `dalang/build_info.hpp`; `CMAKE_CONFIGURE_DEPENDS` re-runs configure when it
  changes.
- At run time: built-in templates, then the file from `--config`, or else the user file
  (`%APPDATA%\dalang\templates.toml`, `$XDG_CONFIG_HOME/dalang/templates.toml` or
  `~/.config/dalang/templates.toml`). A user entry with a built-in name replaces it in place; new
  names are appended. `[[template]]` arrays keep file order, which `dalang list` shows.
- Validation with file and line: missing or empty `name`/`url`, spaces in names, duplicate names,
  malformed `rename`, TOML syntax errors.
- toml++ is built with `TOML_EXCEPTIONS=0`, so parse errors come back as values.

## Alternatives

| Option | Why not |
|---|---|
| JSON | No comments; the file is meant to be edited by hand |
| A file next to the executable | Breaks when the binary is on `PATH` via a package manager; the user config dir is standard |
| User file replaces the built-ins | Users would have to copy every built-in to add one template |
| Placeholders in templates (`{{project_name}}`) | Templates must stay buildable as they are; see ADR 0007 |
| Rename every occurrence in every file | Also hits READMEs and docs that describe the template itself |

## Verification

Tests parse the embedded file (order and rename of `min`, no rename for `zephyr`), every
validation error, merge order, explicit and missing config files, and the user config path.
`dalang new my_app -t full` against GitHub renamed `CMakeLists.txt` and both `.run` files.
