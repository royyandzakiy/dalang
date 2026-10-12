#include "dalang/git.hpp"

#include "dalang/paths.hpp"

#include <fmt/format.h>

#include <utility>

namespace dalang {

namespace {

/// Trims git's output to something that fits in an error message.
std::string trim(std::string_view text) {
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos) {
		return {};
	}
	const auto last = text.find_last_not_of(" \t\r\n");
	return std::string(text.substr(first, last - first + 1));
}

} // namespace

Result<void> Git::check_available() {
	auto result = runner_->run(Command{.args = {"git", "--version"}, .env = {}});
	if (!result) {
		return fail(fmt::format("git is required but {}", result.error().message));
	}
	if (result->exit_code != 0) {
		return fail(fmt::format("`git --version` exited with code {}", result->exit_code));
	}
	return {};
}

Result<void> Git::clone(std::string_view url, const std::filesystem::path &dest, const std::optional<std::string> &ref) {
	std::vector<std::string> args{"git", "clone", "--depth", "1"};
	if (ref && !ref->empty()) {
		args.insert(args.end(), {"--branch", *ref});
	}
	args.insert(args.end(), {std::string(url), to_utf8(dest)});
	return run(std::move(args), "clone");
}

Result<void> Git::init(const std::filesystem::path &dir) {
	return run({"git", "-C", to_utf8(dir), "init", "--quiet"}, "init");
}

Result<void> Git::run(std::vector<std::string> args, std::string_view action) {
	// Without this, git prompts for credentials on a private or mistyped URL and dalang hangs.
	auto result = runner_->run(Command{.args = std::move(args), .env = {{"GIT_TERMINAL_PROMPT", "0"}}});
	if (!result) {
		return fail(result.error().message);
	}
	if (result->exit_code != 0) {
		const auto output = trim(result->output);
		return fail(fmt::format("git {} failed (exit code {}){}{}", action, result->exit_code,
								output.empty() ? "" : ":\n", output));
	}
	return {};
}

} // namespace dalang
