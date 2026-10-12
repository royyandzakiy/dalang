# Contributing

## Changes

1. Branch from `main`.
2. Build with tests on (`-D BUILD_TESTING=ON`) and run `ctest`. New behaviour needs a test in
   `test/`; anything that runs git goes through a runner, so use `FakeRunner` from
   `test/test_utils.hpp` rather than the network.
3. Format with the `format` target (`cmake --build --preset <preset> --target format`). CI runs
   clang-tidy with warnings as errors.
4. Add a line under a new `## [Unreleased]` section of [CHANGELOG.md](CHANGELOG.md).
5. Open a pull request. CI builds and tests on Windows, Linux and macOS.

## Adding a built-in template

Add a `[[template]]` entry to [templates.toml](templates.toml). If the template's project name
appears in its files, list them under `rename`. `TemplateRegistryTest.BuiltinTemplatesParse`
checks the file and the order of names, so update it too.

## Releasing

1. Set the version in [version.txt](version.txt).
2. Rename `## [Unreleased]` in CHANGELOG.md to `## [X.Y.Z] - YYYY-MM-DD` and add the link at the
   bottom.
3. Commit, then tag and push:

   ```bash
   git tag vX.Y.Z
   ```

   ```bash
   git push origin vX.Y.Z
   ```

The [Release workflow](.github/workflows/release.yml) checks that the tag matches version.txt,
builds and tests on all three OSes, and publishes the archives with that CHANGELOG section as
release notes.
