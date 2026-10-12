#pragma once

#include "dalang/error.hpp"

#include <map>
#include <string>
#include <vector>

namespace dalang {

struct Command {
	/// args[0] is the program, looked up on PATH.
	std::vector<std::string> args;
	/// Added to the parent's environment.
	std::map<std::string, std::string> env;
};

struct ProcessResult {
	int exit_code = 0;
	/// stdout and stderr, interleaved.
	std::string output;
};

/// Runs external programs. An interface so tests can fake git without touching the network.
class IProcessRunner {
  public:
	IProcessRunner() = default;
	IProcessRunner(const IProcessRunner &) = delete;
	IProcessRunner &operator=(const IProcessRunner &) = delete;
	IProcessRunner(IProcessRunner &&) = delete;
	IProcessRunner &operator=(IProcessRunner &&) = delete;
	virtual ~IProcessRunner() = default;

	/// Runs `command` to completion. Fails only if the program could not be started;
	/// a non-zero exit code is a successful run with `exit_code != 0`.
	virtual Result<ProcessResult> run(const Command &command) = 0;
};

class ReprocRunner final : public IProcessRunner {
  public:
	/// With `echo_commands`, prints each command line before running it.
	explicit ReprocRunner(bool echo_commands = false) : echo_commands_(echo_commands) {}

	Result<ProcessResult> run(const Command &command) override;

  private:
	bool echo_commands_;
};

} // namespace dalang
