# dalang

Creates new projects from git templates: clones the template, drops its history and README,
renames it to your project and starts a fresh git repository. Like a *dalang*, the Sundanese and
Javanese puppeteer, it brings the templates to life.

## Demo

```text
> dalang new my_app
Cloning 'min' template from https://github.com/royyandzakiy/cpp-project-template-min
Removing template history and README
Renaming 'cpp_project_template' to 'my_app'
Initializing git repository

Created 'my_app' in my_app

> dalang new blinky -t zephyr       # another template
> dalang new fw -t idf -r v1.0      # a template tag or branch
> dalang new -i                     # pick everything in a form
> dalang list                       # available templates
```

## Templates

Built in, from [templates.toml](templates.toml): `min`, `full`, `qml`, `zephyr`, `idf`. Add your
own, or override a built-in one, in `%APPDATA%\dalang	emplates.toml` (Windows) or
`~/.config/dalang/templates.toml` (Linux/macOS), or pass one with `--config`:

```toml
[[template]]
name = "mylib"
url = "https://github.com/me/mylib-template"
description = "My library template"
rename = { token = "mylib_template", files = ["CMakeLists.txt"] }
```

## Architecture

(class diagram / DFD, mermaid based)

## Features

(high level features, or technical API contracts)

## Requirements

## Getting Started

### Clone

### Build

### Run

## Demo Flow

(flow chart / sequence diagram, mermaid based)

## Notes
