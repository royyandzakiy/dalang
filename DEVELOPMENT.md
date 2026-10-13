# DEVELOPMENT

## Done
- Renamed cecep to dalang; `new` / `list` subcommands
- Template registry in `templates.toml` (embedded), user overrides in the config dir
- Project rename via `rename = { token, files }`, `--ref`, `--no-git-init`, `-i` form
- Unit tests with a fake git runner, plus real-git tests against a local repo
- Release workflow on `v*` tags, README, CHANGELOG, LICENSE, CONTRIBUTING

## Next
- Tag v2.0.0 and check the first release run
- winget / Homebrew manifests
- Interactive mode: more choices (C++ standard, package manager) once templates support them
- Connect sanitizer to debug app
