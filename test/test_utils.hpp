// Shared test helpers: a scratch directory, file I/O and a fake process runner.
#pragma once

#include "dalang/core.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

/// A fresh directory under the system temp dir, deleted with its contents on destruction.
class TempDir {
  public:
	TempDir() : m_path(std::filesystem::temp_directory_path() / ("dalang_test_" + std::to_string(std::random_device{}()))) {
		std::filesystem::create_directories(m_path);
	}
	~TempDir() {
		dalang::make_writable(m_path);
		std::error_code err;
		std::filesystem::remove_all(m_path, err);
	}
	TempDir(const TempDir &) = delete;
	TempDir &operator=(const TempDir &) = delete;
	TempDir(TempDir &&) = delete;
	TempDir &operator=(TempDir &&) = delete;

	[[nodiscard]] const std::filesystem::path &path() const { return m_path; }

  private:
	std::filesystem::path m_path;
};

inline void write_file(const std::filesystem::path &file, std::string_view content) {
	std::filesystem::create_directories(file.parent_path());
	std::ofstream(file, std::ios::binary) << content;
}

inline std::string read_file(const std::filesystem::path &file) {
	std::ifstream in(file, std::ios::binary);
	std::ostringstream content;
	content << in.rdbuf();
	return content.str();
}

inline dalang::Result<dalang::ProcessResult> exited(int code, std::string output = {}) {
	return dalang::ProcessResult{.exit_code = code, .output = std::move(output)};
}

/// Records every command and answers with `on_run`, which succeeds by default.
struct FakeRunner {
	std::vector<dalang::Command> calls;
	std::function<dalang::Result<dalang::ProcessResult>(const dalang::Command &)> on_run = [](const auto &) {
		return exited(0);
	};

	dalang::Result<dalang::ProcessResult> run(const dalang::Command &command) {
		calls.push_back(command);
		return on_run(command);
	}
};

static_assert(dalang::ProcessRunnerConcept<FakeRunner>);
