#pragma once

#include "dalang/error.hpp"
#include "dalang/git.hpp"
#include "dalang/templates.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace dalang {

struct GenerateOptions {
	std::filesystem::path destination;
	/// Replaces the template's rename token. Must pass validate_project_name().
	std::string project_name;
	/// Branch or tag of the template; its default branch if unset.
	std::optional<std::string> ref;
	bool init_git = true;
};

/// Receives one line per step, for progress output.
using ProgressFn = std::function<void(std::string_view)>;

/// Creates a new project from a template: clone, drop the template's git history and README,
/// rename, then start a fresh repository.
class Generator {
  public:
	explicit Generator(Git &git, ProgressFn progress = {}) : git_(&git), progress_(std::move(progress)) {}

	/// On failure, removes whatever it created at the destination.
	Result<void> generate(const Template &tpl, const GenerateOptions &options);

  private:
	void report(std::string_view message) const;
	Result<void> populate(const Template &tpl, const GenerateOptions &options);

	Git *git_;
	ProgressFn progress_;
};

/// A project name is a letter followed by letters, digits, '_' or '-', which is valid for
/// CMake's project() and as a folder name on every OS.
Result<void> validate_project_name(std::string_view name);

/// The last component of `destination`, used when no project name is given.
std::string default_project_name(const std::filesystem::path &destination);

/// Replaces every `from` in the file with `to`. Returns the number of replacements.
Result<std::size_t> replace_in_file(const std::filesystem::path &file, std::string_view from, std::string_view to);

} // namespace dalang
