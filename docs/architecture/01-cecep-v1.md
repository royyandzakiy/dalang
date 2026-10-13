# Stage 1: cecep (v1.x)

- **Period:** 2026-06-20 – 2026-10-12
- **Code:** commit `4eb7600` (last cecep commit)
- **Shaped by:** ADR [0001](../adr/0001-git-cli-as-child-process.md); RFC
  [0001](../rfc/0001-cecep-first-generator.md)

## Summary

A personal tool built inside a copy of `cpp-project-template`: one `main.cpp` that maps a
template name to a URL, clones it with git, removes the template's `.git` and README and runs
`git init`. Built from source only; no releases.

## Context (C4 level 1)

```mermaid
flowchart TB
    dev(["Developer<br/>(the author)"]):::person
    cecep["cecep<br/>C++23 CLI, built from source"]:::system
    git["git CLI"]:::external
    gh["GitHub<br/>5 template repositories"]:::external
    fs["Local file system"]:::external

    dev -->|"cecep dest type"| cecep
    cecep -->|"Popen: git clone, git init"| git
    git -->|HTTPS| gh
    cecep -->|"remove .git, README"| fs

    classDef person fill:#08427b,color:#fff,stroke:#052e56
    classDef system fill:#1168bd,color:#fff,stroke:#0b4884
    classDef external fill:#999,color:#fff,stroke:#6b6b6b
```

## Containers (C4 level 2)

One executable, `cecep`, linking CLI11, fmt, cpp-subprocess, FTXUI and toml++ (the last two
unused). No configuration files, no installer.

## Components (C4 level 3)

```mermaid
flowchart LR
    subgraph main["src/main.cpp"]
        tbl["template_type_map<br/>constexpr array"]:::container
        cli["CLI11 parsing<br/>dest, type"]:::container
        clone["clone_template() x2"]:::container
        reset["reset_template_repo()<br/>chdir + git init"]:::container
        menu["select_cpp_standard()<br/>unused"]:::container
    end
    cli --> tbl --> clone --> reset
    classDef container fill:#438dd5,color:#fff,stroke:#2e6295
```

## Key flow

```mermaid
sequenceDiagram
    actor U as User
    participant C as cecep
    participant G as git
    U->>C: cecep my_app min
    C->>C: look up URL (no checks)
    C->>G: git clone URL my_app (output piped, never read)
    G-->>C: exit code
    C->>C: delete my_app/README.md, my_app/.git
    C->>C: chdir my_app
    C->>G: git init
    C->>C: chdir back
    C-->>U: "Project generation successful!"
```

## Known limits

From RFC 0001: `--help` crashed; git errors were lost; no checks before acting and no cleanup;
tests and the vcpkg build could not build; templates required a rebuild to add; generated
projects kept the template's name.
