// Shared types, the process runner and the template registry.
#pragma once

#include <concepts>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dalang {

// ============================================================================
// Errors
// ============================================================================

struct Error {
	std::string message;
};

template <typename T = void> using Result = std::expected<T, Error>;

inline std::unexpected<Error> fail(std::string message) {
	return std::unexpected(Error{std::move(message)});
}

// ============================================================================
// Paths
// ============================================================================

// path::string() and path(std::string) use the ANSI code page on Windows, which mangles
// non-ASCII folder names, so every conversion goes through UTF-8 explicitly.
inline std::string to_utf8(const std::filesystem::path &path) {
	const auto text = path.u8string();
	return {text.begin(), text.end()};
}

inline std::filesystem::path from_utf8(std::string_view text) {
	return {std::u8string(text.begin(), text.end())};
}

// ============================================================================
// Processes
// ============================================================================

struct Command {
	std::vector<std::string> args;			///< args[0] is the program, looked up on PATH
	std::map<std::string, std::string> env; ///< added to the parent's environment
};

struct ProcessResult {
	int exit_code = 0;
	std::string output; ///< stdout and stderr, interleaved
};

/// Anything that can run a Command to completion. It fails only if the program could not be
/// started; a non-zero exit code is a successful run with `exit_code != 0`.
template <typename T>
concept ProcessRunnerConcept = requires(T &runner, const Command &command) {
	{ runner.run(command) } -> std::same_as<Result<ProcessResult>>;
};

/// Runs real processes through reproc.
class ReprocRunner {
  public:
	/// With `echo`, prints each command line and its output.
	explicit ReprocRunner(bool echo = false) : m_echo(echo) {
	}

	[[nodiscard]] Result<ProcessResult> run(const Command &command) const;

  private:
	bool m_echo;
};

static_assert(ProcessRunnerConcept<ReprocRunner>);

// ============================================================================
// Templates
// ============================================================================

/// Where the template's own project name appears, so it can be swapped for the new one.
struct Rename {
	std::string token;
	std::vector<std::string> files; ///< relative to the generated project's root
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
	[[nodiscard]] std::span<const Template> all() const {
		return m_templates;
	}
	[[nodiscard]] std::vector<std::string> names() const;

  private:
	std::vector<Template> m_templates;
};

/// The per-user templates.toml: %APPDATA%\dalang on Windows, $XDG_CONFIG_HOME/dalang or
/// ~/.config/dalang elsewhere. Empty if the base directory can't be determined.
std::filesystem::path user_config_path();

/// Built-in templates merged with `config_path` if given (it must exist), otherwise with the
/// user config file if it exists.
Result<TemplateRegistry> load_registry(const std::optional<std::filesystem::path> &config_path);

// ============================================================================
// Project files
// ============================================================================

/// A letter followed by letters, digits, '_' or '-': valid for CMake's project() and as a
/// folder name on every OS.
Result<void> validate_project_name(std::string_view name);

/// The last component of `destination`, used when no project name is given.
std::string default_project_name(const std::filesystem::path &destination);

/// Replaces every `from` in the file with `to`. Returns the number of replacements.
Result<std::size_t> replace_in_file(const std::filesystem::path &file, std::string_view from, std::string_view to);

/// Clears read-only flags under `dir` so remove_all() can delete git's object files on Windows.
void make_writable(const std::filesystem::path &dir);

} // namespace dalang
