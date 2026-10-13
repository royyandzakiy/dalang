# RFC 0002: Rename cecep to dalang

- **Status:** Implemented
- **Date:** 2026-10-13
- **Commits:** `a770d10`
- **ADR:** [0002](../adr/0002-name-dalang.md)

## Summary

Rename the tool and the project from cecep to dalang before the first public release, while the
only user is the author and the cost of a rename is lowest.

## Findings

- **Pronunciation.** In Sundanese "c" is "ch", so cecep reads "che-chep". English readers say
  "see-sep" or "kee-kep", French readers "sesep". The "cpp" pun is lost on all of them.
- **Scope.** Two of five templates (`zephyr`, `idf`) are firmware and one (`qml`) is a Qt app.
  A name built around "cpp" undersells the tool and will fit worse as templates are added.

## Alternatives

| Name | Meaning | Why not |
|---|---|---|
| **dalang** (chosen) | The puppeteer in wayang, who brings the puppets to life | – |
| pola | "Pattern" or "template" in Indonesian | Accurate, but plain; likely taken in package registries |
| bibit / tunas | Seed / sprout | Good metaphor, weaker link to what the tool does |
| cetak | To mold or print | Same "c" pronunciation problem as cecep |
| golek | Wayang golek, the Sundanese wooden puppet | Clearly Sundanese, meaning opaque to outsiders |
| kiln, sprout, scaff | English, descriptive | Lose the Sundanese identity |
| cppinit, cpp-new | Descriptive | Generic and C++-only |

## Implementation

- `project(dalang ...)` with `DESCRIPTION` and `HOMEPAGE_URL`; the executable is `dalang`.
- CLion run configs renamed (`.run/dalang.run.xml`, `.run/dalang_tests.run.xml`); VS Code and
  `EDITOR_SETUP.md` refer to `dalang` / `dalang_tests`.
- README, CHANGELOG, `vcpkg.json` (`"name": "dalang"`) and `conanfile.py` (`DalangConan`) renamed.
- Version 2.0.0, since the command line also changed (RFC [0006](0006-command-line-v2.md)).

## Follow-ups

- Rename the GitHub repository `royyandzakiy/cecep` to `royyandzakiy/dalang`. GitHub redirects
  the old URL; the README, CHANGELOG and generated READMEs already use the new one.
- The local working folder is still `cecep/`; renaming it is cosmetic.
