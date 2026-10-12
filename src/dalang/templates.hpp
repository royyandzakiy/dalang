#pragma once

#include "dalang/error.hpp"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace dalang {

/// Where the template's own project name appears, so it can be swapped for the new one.
struct Rename {
	std::string token;
	/// Relative to the generated project's root.
	std::vector<std::string> files;
};

struct Template {
	std::string name;
	std::string url;
	std::string description;
	std::optional<Rename> rename;
};

/// An ordered set of templates with unique names.
class TemplateRegistry {
  public:
	/// Parses `[[template]]` entries from TOML. `source` names the input in error messages.
	static Result<TemplateRegistry> parse(std::string_view toml, std::string_view source);
	static Result<TemplateRegistry> load_file(const std::filesystem::path &path);
	/// The templates.toml embedded at build time.
	static Result<TemplateRegistry> builtin();

	/// Adds the templates from `other`; one with an existing name replaces it in place.
	void merge(const TemplateRegistry &other);

	[[nodiscard]] const Template *find(std::string_view name) const;
	[[nodiscard]] std::span<const Template> all() const { return templates_; }
	[[nodiscard]] std::vector<std::string> names() const;

  private:
	std::vector<Template> templates_;
};

/// The per-user templates.toml: %APPDATA%\dalang on Windows, $XDG_CONFIG_HOME/dalang or
/// ~/.config/dalang elsewhere. Empty if the base directory can't be determined.
std::filesystem::path user_config_path();

/// Built-in templates merged with `config_path` if given (it must exist), otherwise with the
/// user config file if it exists.
Result<TemplateRegistry> load_registry(const std::optional<std::filesystem::path> &config_path);

} // namespace dalang
