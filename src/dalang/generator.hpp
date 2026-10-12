// Git operations and project generation. Templated over the process runner, so tests can pass
// any type with a matching run() instead of spawning git.
#pragma once

#include "dalang/core.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace dalang {

// ============================================================================
// Git
// ============================================================================

namespace git {

namespace detail {

inline std::string trim(std::string_view text) {
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos) {
		return {};
	}
	return std::string(text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1));
}

template <ProcessRunnerConcept RunnerT>
Result<void> run(RunnerT &runner, std::vector<std::string> args, std::string_view action) {
	// Without this, git prompts for credentials on a private or mistyped URL and dalang hangs.
	return runner.run(Command{.args = std::move(args), .env = {{"GIT_TERMINAL_PROMPT", "0"}}})
		.and_then([&](const ProcessResult &result) -> Result<void> {
			if (result.exit_code == 0) {
				return {};
			}
			const auto output = trim(result.output);
			return fail(fmt::format("git {} failed (exit code {}){}{}", action, result.exit_code,
									output.empty() ? "" : ":\n", output));
		});
}

} // namespace detail

template <ProcessRunnerConcept RunnerT>
Result<void> check_available(RunnerT &runner) {
	return runner.run(Command{.args = {"git", "--version"}, .env = {}})
		.transform_error([](const Error &err) { return Error{fmt::format("git is required but {}", err.message)}; })
		.and_then([](const ProcessResult &result) -> Result<void> {
			if (result.exit_code != 0) {
				return fail(fmt::format("`git --version` exited with code {}", result.exit_code));
			}
			return {};
		});
}

/// Shallow clone of `url` into `dest`. `ref` is a branch or tag; the default branch if unset.
template <ProcessRunnerConcept RunnerT>
Result<void> clone(RunnerT &runner, std::string_view url, const std::filesystem::path &dest,
				   const std::optional<std::string> &ref) {
	std::vector<std::string> args{"git", "clone", "--depth", "1"};
	if (ref && !ref->empty()) {
		args.insert(args.end(), {"--branch", *ref});
	}
	args.insert(args.end(), {std::string(url), to_utf8(dest)});
	return detail::run(runner, std::move(args), "clone");
}

template <ProcessRunnerConcept RunnerT>
Result<void> init(RunnerT &runner, const std::filesystem::path &dir) {
	return detail::run(runner, {"git", "-C", to_utf8(dir), "init", "--quiet"}, "init");
}

} // namespace git

// ============================================================================
// Generator
// ============================================================================

struct GenerateOptions {
	std::filesystem::path destination;
	std::string project_name; ///< replaces the template's rename token
	std::optional<std::string> ref; ///< branch or tag; the template's default branch if unset
	bool init_git = true;
};

/// Receives one line per step, for progress output.
using ProgressFn = std::function<void(std::string_view)>;

/// Creates a project from a template: clone, drop the template's history and README, rename,
/// then start a fresh repository. On failure, removes whatever it created at the destination.
template <ProcessRunnerConcept RunnerT = ReprocRunner>
class Generator {
  public:
	explicit Generator(RunnerT runner = RunnerT{}, ProgressFn progress = {})
		: m_runner(std::move(runner)), m_progress(std::move(progress)) {}

	Result<void> generate(const Template &tpl, const GenerateOptions &options) {
		const auto &dest = options.destination;
		std::error_code err;
		const bool existed = std::filesystem::exists(dest, err);

		return validate_project_name(options.project_name)
			.and_then([&] { return check_destination(dest, existed); })
			.and_then([&] { return git::check_available(m_runner); })
			.and_then([&] {
				return populate(tpl, options).or_else([&](Error error) -> Result<void> {
					clean_up(dest, existed);
					return std::unexpected(std::move(error));
				});
			});
	}

	RunnerT &runner() { return m_runner; }

  private:
	static Result<void> check_destination(const std::filesystem::path &dest, bool existed) {
		std::error_code err;
		if (!existed) {
			return {};
		}
		if (!std::filesystem::is_directory(dest, err)) {
			return fail(fmt::format("destination '{}' exists and is not a directory", to_utf8(dest)));
		}
		if (!std::filesystem::is_empty(dest, err)) {
			return fail(fmt::format("destination '{}' already exists and is not empty", to_utf8(dest)));
		}
		return {};
	}

	/// Removes `dest` if dalang created it, otherwise only its contents.
	static void clean_up(const std::filesystem::path &dest, bool existed) {
		std::error_code err;
		make_writable(dest);
		if (!existed) {
			std::filesystem::remove_all(dest, err);
			return;
		}
		for (const auto &entry : std::filesystem::directory_iterator(dest, err)) {
			std::filesystem::remove_all(entry.path(), err);
		}
	}

	static Result<void> write_readme(const std::filesystem::path &file, const GenerateOptions &options,
									 const Template &tpl) {
		std::ofstream out(file, std::ios::binary);
		if (!(out << fmt::format("# {}\n\nGenerated by [dalang](https://github.com/royyandzakiy/dalang) from the "
								 "`{}` template ({}).\n",
								 options.project_name, tpl.name, tpl.url))) {
			return fail(fmt::format("cannot write '{}'", to_utf8(file)));
		}
		return {};
	}

	Result<void> rename(const Template &tpl, const GenerateOptions &options) {
		if (!tpl.rename) {
			return {};
		}
		report(fmt::format("Renaming '{}' to '{}'", tpl.rename->token, options.project_name));
		for (const auto &file : tpl.rename->files) {
			const auto path = options.destination / from_utf8(file);
			std::error_code err;
			if (!std::filesystem::exists(path, err)) {
				report(fmt::format("  skipped {} (not in template)", file));
				continue;
			}
			if (auto replaced = replace_in_file(path, tpl.rename->token, options.project_name); !replaced) {
				return std::unexpected(std::move(replaced).error());
			}
		}
		return {};
	}

	Result<void> populate(const Template &tpl, const GenerateOptions &options) {
		const auto &dest = options.destination;
		report(fmt::format("Cloning '{}' template from {}{}", tpl.name, tpl.url,
						   options.ref ? fmt::format(" ({})", *options.ref) : ""));

		return git::clone(m_runner, tpl.url, dest, options.ref)
			.and_then([&] -> Result<void> {
				report("Removing template history and README");
				const auto git_dir = dest / ".git";
				make_writable(git_dir);
				std::error_code err;
				std::filesystem::remove_all(git_dir, err);
				if (err) {
					return fail(fmt::format("cannot remove '{}': {}", to_utf8(git_dir), err.message()));
				}
				return write_readme(dest / "README.md", options, tpl);
			})
			.and_then([&] { return rename(tpl, options); })
			.and_then([&] -> Result<void> {
				if (!options.init_git) {
					return {};
				}
				report("Initializing git repository");
				return git::init(m_runner, dest);
			});
	}

	void report(std::string_view message) const {
		if (m_progress) {
			m_progress(message);
		}
	}

	RunnerT m_runner;
	ProgressFn m_progress;
};

} // namespace dalang
