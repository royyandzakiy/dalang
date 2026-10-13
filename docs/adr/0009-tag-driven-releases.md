# ADR 0009: Releases are built by GitHub Actions from a version tag

- **Status:** Accepted
- **Date:** 2026-10-13
- **RFC:** [0007](../rfc/0007-releases-and-docs.md)

## Context

There were no published binaries; users had to install a C++23 toolchain and Conan or vcpkg to
try the tool.

## Decision

- Pushing `vX.Y.Z` runs `.github/workflows/release.yml`: check the tag equals `version.txt`,
  build and test `msvc-release`, `clang-linux-release` and `appleclang-release`, `cmake --install`,
  archive, then `gh release create` with SHA256 sums and the CHANGELOG section as notes.
- `version.txt` stays the single source of the version (CMake, `--version`, the tag check).

## Alternatives considered

- CPack (more machinery for one executable); releasing on every push to main (versions lose
  meaning); a third-party release action (`gh` is preinstalled).

## Consequences

- A release is three steps: bump `version.txt`, add the CHANGELOG section, push the tag.
- A tag that disagrees with `version.txt`, or a missing CHANGELOG section, fails the release
  before anything is published.
