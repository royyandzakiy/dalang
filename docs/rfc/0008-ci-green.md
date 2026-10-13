# RFC 0008: Getting CI green on all 17 jobs

- **Status:** Implemented
- **Date:** 2026-10-13
- **ADR:** –
- **Related:** RFC [0003](0003-honest-build.md), RFC [0007](0007-releases-and-docs.md)

## Summary

CI had failed on every run since July 2026. Run #33 (`078428d`) failed 12 of 17 jobs, and the
first `v2.0.0` release run failed on Windows. Five separate causes, none in dalang's own logic;
each fix is small.

## Findings

| Jobs | Symptom | Cause |
|---|---|---|
| `clang-linux-asan`, `clang-linux-full` | `mold: undefined symbol: __lsan_do_recoverable_leak_check` linking `sanitizer_tests` | `sanitizer_tests` never called `target_enable_sanitizers()`, so it was built without `-fsanitize` while its source expected LSan. Inherited from the template. |
| `clang-linux-tsan` | `SanitizerDeathTest.UndefinedBehavior` and `.ThreadSanitizer` did not die | Same cause: the trip-wires ran uninstrumented. |
| `clang-cl-debug`, `clang-cl-release` | `fmt/12.1.0: Error in generate() method, line 99` during Conan install | The runner image moved to Visual Studio 2026 (VS 18, MSVC 14.51). The clang-cl profiles pinned `compiler.runtime_version=v143` (VS 2022), so `CMakeToolchain.generate()` looked for a VS 17 install for its build environment and found none. |
| `msvc-asan` | `LNK2038: mismatch detected for 'annotate_optional'` | The VS 2026 STL added an ASan annotation for `std::optional`. Our ASan setup only disabled the string and vector annotations, so instrumented code and the prebuilt Conan libraries disagreed. |
| `mingw-debug`, `mingw-release` | Test discovery: `Exit code 0xc0000139` | `STATUS_ENTRYPOINT_NOT_FOUND`: `dalang_tests.exe` loaded a different MinGW's `libstdc++-6.dll` from the runner's `PATH`. |
| `msvc-debug`, `msvc-release`, release `dalang-windows-x64` | `RealGitTest`: `"project(my_app)\r\n# v2\r\n"` vs `"...\n"` | The Windows runner's git has `core.autocrlf=true`, so the clone checks files out with CRLF. dalang is right to follow the user's git config; the test assumed LF. |

## Implementation

1. `test/CMakeLists.txt`: `target_enable_sanitizers(sanitizer_tests)`; the explicit
   `deploy_asan_runtime()` is dropped, since the helper already does it on Windows.
2. `cmake/conan-profiles/clang-cl-windows-{debug,release,asan}.ini`: Jinja-rendered;
   `runtime_version` is `v145` when the dev shell's `VCToolsVersion` is 14.50 or newer, else
   `v143`. The ASan profile had no `runtime_version` at all and now gets the same line.
3. `cmake/sanitizers.cmake`: `_DISABLE_STL_ANNOTATION` instead of the string/vector macros, which
   also covers annotations later STLs add.
4. `cmake/compiler.cmake`: `-static` link option on MinGW, so executables carry their own
   libstdc++, libgcc and winpthread.
5. `test/test_generator.cpp`: real-git tests compare file contents with CR removed.

## Verification

- Conan profile rendering: `VCToolsVersion=14.51.36231` gives `v145`, `14.44.35207` and unset give
  `v143`.
- `clang-cl-debug` on Windows (VS 2022): build clean, 32/32 tests.
- WSL Ubuntu 26.04, Clang 21, mold: `clang-linux-asan` 35/35 tests including the three trip-wires.
- Not reproducible locally: `msvc-asan` and the VS 2026 clang-cl profiles (only VS 2022 here);
  the next CI run checks them.

## Follow-ups

- The template repositories (`cpp-project-template`, `-min`, `qml`) share these CMake modules and
  profiles and likely fail the same way; port the five fixes there.
