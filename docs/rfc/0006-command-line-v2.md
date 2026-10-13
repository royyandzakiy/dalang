# RFC 0006: Command line v2 and safe generation

- **Status:** Implemented
- **Date:** 2026-10-13
- **Commits:** `a770d10`, `1d4ec2b`, `6d8c901`
- **ADR:** [0008](../adr/0008-subcommand-cli.md)

## Summary

Replace `cecep <dest> <type>` with `dalang new` / `dalang list`, add the options a generator needs
(name, ref, no init, interactive, verbosity, config), and make generation refuse or undo instead
of leaving a broken folder.

## Findings

From RFC [0001](0001-cecep-first-generator.md): `--help` crashed (1), git errors were lost (2),
the working directory change was not exception safe (3), and nothing was checked or cleaned up
(4). Also: there was no way to pick a template version, no way to list templates, and the
interactive menu was dead code.

## Architecture

```text
dalang [-v | -q] [-c FILE] [-V] new <destination> [-t NAME] [-n NAME] [-r REF] [--no-git-init] [-i]
dalang [-v | -q] [-c FILE] list
```

Generation order and guarantees:

1. **Before anything runs:** the project name must match `[A-Za-z][A-Za-z0-9_-]*` (valid for
   CMake `project()` and as a folder); the destination must not exist or be an empty directory;
   `git --version` must succeed.
2. **Clone:** `git clone --depth 1 [--branch REF] URL DEST` with `GIT_TERMINAL_PROMPT=0`, so a
   private or mistyped URL fails instead of waiting for a password.
3. Remove `.git` (after clearing read-only flags, which git sets on Windows object files), write a
   README naming the template, apply the rename (RFC [0005](0005-template-registry.md)).
4. `git -C DEST init --quiet`, unless `--no-git-init`.
5. **On any failure after step 1:** remove the destination if dalang created it, otherwise empty
   it again.

Output: one progress line per step; `-q` prints errors only; `-v` echoes each git command and its
output. Errors are one `error: ...` line on stderr, including git's own message. Exit code 0 or 1.

`-i` opens an FTXUI form (template radio list, destination, project name); Enter or *Create*
confirms, Esc or *Cancel* exits with code 1.

## Alternatives

| Option | Why not |
|---|---|
| Keep positional `dest type` | Ambiguous next to new options; no room for `list` |
| `--force` to overwrite a non-empty folder | Deletes user data on a typo; refusing is safer |
| `chdir` into the destination for `git init` | Process-wide state; `git -C` needs none |
| Colored output | Escape codes in pipes and CI logs; plain text is enough |

## Verification

- Every failure path has a `FakeRunner` test (git missing, clone or init failing, non-empty or
  file destination, invalid name) asserting the error and what is left on disk.
- Real-git tests: clone and rename, tag checkout with `--ref`, unknown repository leaves nothing.
- Manual runs of the built binary on Windows: `--help`, `-V`, `list`, `new` for `min`, `full` and
  `idf` against GitHub, non-empty destination, unknown template, invalid name, unknown ref with
  `-v`, `-q`.
- Not verified: the `-i` form, which needs an interactive terminal.
