# DEVELOPMENT

## Done
- Renamed cecep to dalang; `new` / `list` subcommands
- Template registry in `templates.toml` (embedded), user overrides in the config dir
- Project rename via `rename = { token, files }`, `--ref`, `--no-git-init`, `-i` form
- Unit tests with a fake git runner, plus real-git tests against a local repo

## Next
- GitHub workflow to publish release binaries on `v*` tags
- README rewrite (install, usage, templates, flow diagram), fresh CHANGELOG, LICENSE
- Interactive mode: more choices (C++ standard, package manager) once templates support them
- Connect sanitizer to debug app
