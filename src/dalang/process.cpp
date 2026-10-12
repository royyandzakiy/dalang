#include "dalang/process.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

#include <system_error>

namespace dalang {

Result<ProcessResult> ReprocRunner::run(const Command &command) {
	if (command.args.empty()) {
		return fail("empty command");
	}
	const auto &program = command.args.front();
	if (echo_commands_) {
		fmt::println("$ {}", fmt::join(command.args, " "));
	}

	reproc::options options;
	options.redirect.in.type = reproc::redirect::discard;
	options.env.behavior = reproc::env::extend;
	options.env.extra = command.env;

	reproc::process process;
	if (const auto err = process.start(command.args, options)) {
		if (err == std::errc::no_such_file_or_directory) {
			return fail(fmt::format("'{}' was not found on PATH", program));
		}
		return fail(fmt::format("failed to start '{}': {}", program, err.message()));
	}

	std::string output;
	reproc::sink::string sink(output);
	if (const auto err = reproc::drain(process, sink, sink)) {
		return fail(fmt::format("failed to read output of '{}': {}", program, err.message()));
	}

	const auto [status, err] = process.wait(reproc::infinite);
	if (err) {
		return fail(fmt::format("failed to wait for '{}': {}", program, err.message()));
	}
	return ProcessResult{.exit_code = status, .output = std::move(output)};
}

} // namespace dalang
