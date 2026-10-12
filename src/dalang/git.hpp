#pragma once

#include "dalang/error.hpp"
#include "dalang/process.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dalang {

/// The few git operations dalang needs, run through an IProcessRunner.
class Git {
  public:
	explicit Git(IProcessRunner &runner) : runner_(&runner) {}

	Result<void> check_available();

	/// Shallow clone of `url` into `dest`. `ref` is a branch or tag; the default branch if empty.
	Result<void> clone(std::string_view url, const std::filesystem::path &dest, const std::optional<std::string> &ref);

	Result<void> init(const std::filesystem::path &dir);

  private:
	Result<void> run(std::vector<std::string> args, std::string_view action);

	IProcessRunner *runner_;
};

} // namespace dalang
