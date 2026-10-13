# RFC 0003: An honest build: template leftovers, reproc, clang-tidy, coverage

- **Status:** Implemented
- **Date:** 2026-10-13
- **Commits:** `a770d10`, `6d8c901`
- **ADR:** [0003](../adr/0003-reproc-for-processes.md)

## Summary

Make every configuration the repository advertises actually build and test: remove what the
template left behind, replace the process library that blocked vcpkg, fix clang-tidy on
clang-cl, and turn coverage back on in CI.

## Findings

1. **Tests could not build** (RFC [0001](0001-cecep-first-generator.md), finding 6): `app_tests`
   and the `MyLib` tests referenced sources that were not in the repository.
2. **vcpkg could not build.** cpp-subprocess has no vcpkg port, and `vcpkg.json` did not list it.
3. **clang-tidy rejected every `try` on clang-cl.** With `ENABLE_CLANG_TIDY=ON`, CMake runs
   clang-tidy in `cl` driver mode; it dropped the `/EHsc` from the compile command and reported
   `cannot use 'try' with exceptions disabled`. Running the same command with
   `--extra-arg=/EHsc` passes. cecep never hit it because its `main.cpp` had no `try`.
4. **clang-tidy false positive in the MSVC STL.** `clang-analyzer-optin.core.EnumCastOutOfRange`
   fires on `_BITMASK_OPS(__std_fs_stats_flags)` in `<xfilesystem_abi.h>` for any file that
   uses `<filesystem>`, and `WarningsAsErrors: '*'` turns it into a build failure.
5. **Coverage was commented out in CI**, but the badge step still read `coverage.lcov`, so the
   `clang-linux-debug` leg would fail once it ran. The coverage ignore regex also did not exclude
   the Conan cache or generated headers.

## Architecture

Unchanged; this RFC is build hygiene. Removed: `examples/mylib_consumer`,
`cmake/MyLibConfig.cmake.in`, `cmake/version.h.in` and `generate_version_header()`, the
`BUILD_LIB` / `BUILD_LIB_SHARED` / `GENERATE_VERSION_HEADER` options, the four `*-lib-only`
presets, and the MyLib / greeter tests.

## Alternatives

| Problem | Option | Why not |
|---|---|---|
| Process library | Keep cpp-subprocess, FetchContent it for vcpkg | Two dependency paths for one library; its `Popen` + `wait()` pattern deadlocks on full pipes unless every caller uses `communicate()` |
| | Boost.Process | Pulls Boost into a small CLI |
| | Hand-written CreateProcess / posix_spawn | More code to own and test on three OSes |
| | **reproc** (chosen) | On Conan Center and vcpkg, both expose the `reproc++` target; `drain()` reads both pipes |
| clang-tidy `/EHsc` | Drop `try` from first-party code | CLI11 reports `--help` through exceptions; catching them is the documented use |
| STL false positive | `NOLINT` at each use | It fires inside the STL header, not at a use site |

## Implementation (as executed)

1. Delete the leftovers above; `src/CMakeLists.txt` and `test/CMakeLists.txt` rewritten for the
   real targets (`dalang`, `dalang_internal`, `dalang_tests`).
2. `conanfile.py` and `vcpkg.json`: `cpp-subprocess` and the unused `gtest` replaced by
   `reproc/14.2.5`; `project_dependencies.cmake`: `find_package(reproc++ CONFIG REQUIRED)`.
3. `cmake/configure_target.cmake`: on `MSVC`, the clang-tidy command gets `--extra-arg=/EHsc`.
4. `.clang-tidy`: `-clang-analyzer-optin.core.EnumCastOutOfRange`, with a one-line reason.
5. CI: coverage report, job summary and HTML upload re-enabled for `clang-linux-debug`;
   `cmake/coverage.cmake` ignores `/[.]conan2/`, `/vcpkg_installed/` and `/generated/`.

## Verification

- `clang-cl-debug` with `BUILD_TESTING=ON` and clang-tidy on: clean build, no warnings, all tests
  pass.
- reproc's Conan package and the vcpkg port both generate `reproc++` config files with a
  `reproc++` target (checked from the Conan install and `ports/reproc/portfile.cmake`).

## Follow-ups

- A full `PKG_MANAGER=vcpkg` build was not run locally, because the author's
  `project_options.local.cmake` forces Conan; CI's manual `workflow_dispatch` input covers it.
- Linux and macOS CI had not run at the time of writing.
