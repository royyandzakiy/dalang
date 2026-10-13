# Stage 2: Current (v2.0.0+), living document

- **Period:** 2026-10-13 – now (last updated 2026-10-13)
- **Code:** this repository, `HEAD`
- **Shaped by:** ADR [0002](../adr/0002-name-dalang.md) – [0009](../adr/0009-tag-driven-releases.md);
  RFC [0002](../rfc/0002-rename-to-dalang.md) – [0007](../rfc/0007-releases-and-docs.md)

> Update this file when the architecture changes; see [Updating](README.md#updating).

## Summary

dalang is a single executable with no files beside it. A small core (three files) loads a
template registry from TOML, validates the request, and drives the git CLI through a runner that
is a template parameter, so tests replace git without interfaces. Releases are built for three
OSes from a version tag.

## Context (C4 level 1)

```mermaid
flowchart TB
    dev(["Developer"]):::person
    maint(["Maintainer"]):::person
    dalang["dalang v2<br/>C++23 CLI"]:::system
    git["git CLI<br/>user's credentials, SSH, proxies"]:::external
    hosts["Template hosts<br/>GitHub or any git URL"]:::external
    fs["Local file system<br/>destination, user templates.toml"]:::external
    gha["GitHub Actions<br/>CI + Release"]:::external
    rel["GitHub Releases"]:::external

    dev -->|"dalang new / list"| dalang
    dalang -->|"child process: clone, init"| git
    git --> hosts
    dalang -->|"read config, write project"| fs
    maint -->|"push, tag vX.Y.Z"| gha
    gha -->|"archives + SHA256SUMS"| rel
    dev -->|download| rel

    classDef person fill:#08427b,color:#fff,stroke:#052e56
    classDef system fill:#1168bd,color:#fff,stroke:#0b4884
    classDef external fill:#999,color:#fff,stroke:#6b6b6b
```

## Containers (C4 level 2): what is delivered

```mermaid
flowchart LR
    subgraph archive["dalang-&lt;os&gt;.zip / .tar.gz = cmake --install tree"]
        exe["bin/dalang<br/>templates.toml and version embedded"]:::container
        docs["README, LICENSE, CHANGELOG,<br/>templates.toml (reference copy)"]:::container
    end
    user["%APPDATA%/dalang/templates.toml<br/>~/.config/dalang/templates.toml"]:::external
    exe -->|"merged over built-ins"| user

    classDef container fill:#438dd5,color:#fff,stroke:#2e6295
    classDef external fill:#999,color:#fff,stroke:#6b6b6b
```

Release builds use Conan, whose packages are static by default, so the dependencies (fmt, CLI11,
FTXUI, toml++, reproc) are inside the binary; on Windows it needs the VC++ runtime. A vcpkg build
on Windows uses DLLs (dynamic triplet).

## Components (C4 level 3)

```mermaid
flowchart TB
    subgraph exe["dalang executable"]
        main["main.cpp<br/>CLI11 subcommands, FTXUI form,<br/>progress + error output"]:::container
        subgraph core["dalang_internal (OBJECT library)"]
            reg["TemplateRegistry<br/>parse / merge / find (toml++)"]:::container
            gen["Generator&lt;RunnerT&gt;<br/>validate -> clone -> clean -> rename -> init,<br/>cleanup on failure"]:::container
            gitf["git::check_available / clone / init<br/>GIT_TERMINAL_PROMPT=0"]:::container
            run["ReprocRunner<br/>satisfies ProcessRunnerConcept"]:::container
            files["validate_project_name, replace_in_file,<br/>make_writable, UTF-8 paths"]:::container
            info["build_info.hpp (generated)<br/>version, embedded templates.toml"]:::container
        end
    end
    main --> reg
    main --> gen
    reg --> info
    gen --> gitf --> run
    gen --> files
    tests["dalang_tests<br/>FakeRunner, real-git tests"]:::external
    tests -.-> core

    classDef container fill:#438dd5,color:#fff,stroke:#2e6295
    classDef external fill:#999,color:#fff,stroke:#6b6b6b
```

| Component | File |
|---|---|
| Error / Result, paths, Command, runner concept, `ReprocRunner`, `TemplateRegistry`, file helpers | `src/dalang/core.hpp`, `core.cpp` |
| `git::*`, `GenerateOptions`, `Generator<RunnerT>` | `src/dalang/generator.hpp` |
| Version and embedded templates | `src/dalang/build_info.hpp.in` -> build tree |
| Command line and form | `src/main.cpp` |

## Key flow: `dalang new my_app -t min`

```mermaid
sequenceDiagram
    actor U as User
    participant M as main.cpp
    participant R as TemplateRegistry
    participant G as Generator
    participant X as git (via runner)
    participant F as File system

    U->>M: dalang new my_app -t min
    M->>R: load_registry(config)
    R-->>M: built-ins + user file
    M->>G: generate(min, {my_app, name, ref, init})
    G->>G: validate_project_name
    G->>F: destination missing or empty?
    G->>X: git --version
    G->>X: git clone --depth 1 [--branch ref] url my_app
    G->>F: remove .git, write README
    G->>F: replace token in rename.files
    G->>X: git -C my_app init --quiet
    alt any step after the checks fails
        G->>F: remove my_app (or empty it if it existed)
        G-->>M: Error (with git's output)
        M-->>U: error: ... (exit 1)
    else success
        G-->>M: ok
        M-->>U: Created 'my_app' (exit 0)
    end
```

## What changed from stage 1

| Change | RFC / ADR |
|---|---|
| Renamed to dalang; version 2.0.0 | RFC 0002, ADR 0002 |
| Template leftovers removed; tests and vcpkg build; reproc instead of cpp-subprocess | RFC 0003, ADR 0003 |
| One file -> core library + header-only generator over `ProcessRunnerConcept`; `std::expected` | RFC 0004, ADR 0004, 0005 |
| Hardcoded table -> embedded `templates.toml` + user file; project rename | RFC 0005, ADR 0006, 0007 |
| Positional CLI -> `new` / `list`; checks before acting; cleanup on failure | RFC 0006, ADR 0008 |
| Release archives from tags; docs | RFC 0007, ADR 0009 |

## Known limits

- The `-i` form has no automated test; it needs an interactive terminal.
- Rename covers only the files a template lists.
- No offline mode: every `new` clones from the network (or a local `file://` URL).
- `fmt` instead of `std::print` until CI's standard library has `<print>` (ADR 0005).
