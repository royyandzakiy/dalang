#include "dalang/generator.hpp"
#include "test_utils.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace fs = std::filesystem;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::Pair;

namespace {

/// The git subcommand of a command line, skipping `-C <dir>`.
std::string subcommand(const dalang::Command &command) {
	const auto &args = command.args;
	return args.at(1) == "-C" ? args.at(3) : args.at(1);
}

std::vector<std::string> subcommands(const FakeRunner &runner) {
	std::vector<std::string> result;
	for (const auto &command : runner.calls) {
		result.push_back(subcommand(command));
	}
	return result;
}

dalang::Template sample_template(std::string url = "https://example.com/sample.git") {
	return {
		.name = "sample",
		.url = std::move(url),
		.description = "",
		.rename = dalang::Rename{.token = "old_name", .files = {"CMakeLists.txt", "not_in_template.txt"}},
	};
}

dalang::GenerateOptions options_for(const fs::path &dest, std::string name = "my_app") {
	return {.destination = dest, .project_name = std::move(name), .ref = std::nullopt, .init_git = true};
}

/// A runner whose `clone` writes a small template tree, and whose `failing` subcommand fails.
FakeRunner fake_git(std::string failing = {}) {
	FakeRunner runner;
	runner.on_run = [failing = std::move(failing)](const dalang::Command &command) {
		if (subcommand(command) == failing) {
			return exited(1, "boom");
		}
		if (subcommand(command) == "clone") {
			const fs::path dest = command.args.back();
			write_file(dest / ".git" / "HEAD", "ref: refs/heads/main\n");
			write_file(dest / "README.md", "# Template readme\n");
			write_file(dest / "CMakeLists.txt", "project(\n  old_name\n)\n# old_name\n");
		}
		return exited(0);
	};
	return runner;
}

} // namespace

// ============================================================================
// git
// ============================================================================

TEST(GitTest, CloneIsShallowAndNeverPrompts) {
	FakeRunner runner;
	ASSERT_TRUE(dalang::git::clone(runner, "https://x/t.git", "dest", std::nullopt));
	ASSERT_EQ(runner.calls.size(), 1U);
	EXPECT_THAT(runner.calls[0].args, ElementsAre("git", "clone", "--depth", "1", "https://x/t.git", "dest"));
	EXPECT_THAT(runner.calls[0].env, Contains(Pair("GIT_TERMINAL_PROMPT", "0")));
}

TEST(GitTest, ClonePassesRefAsBranch) {
	FakeRunner runner;
	ASSERT_TRUE(dalang::git::clone(runner, "https://x/t.git", "dest", "v1.2"));
	EXPECT_THAT(runner.calls.at(0).args,
				ElementsAre("git", "clone", "--depth", "1", "--branch", "v1.2", "https://x/t.git", "dest"));
}

TEST(GitTest, FailureIncludesGitOutput) {
	FakeRunner runner;
	runner.on_run = [](const auto &) { return exited(128, "fatal: repository not found\n"); };

	auto result = dalang::git::clone(runner, "https://x/missing.git", "dest", std::nullopt);
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("exit code 128"));
	EXPECT_THAT(result.error().message, HasSubstr("fatal: repository not found"));
}

TEST(GitTest, InitRunsInsideTheDirectory) {
	FakeRunner runner;
	ASSERT_TRUE(dalang::git::init(runner, "dest"));
	EXPECT_THAT(runner.calls.at(0).args, ElementsAre("git", "-C", "dest", "init", "--quiet"));
}

TEST(GitTest, MissingGitIsReported) {
	FakeRunner runner;
	runner.on_run = [](const auto &) -> dalang::Result<dalang::ProcessResult> {
		return dalang::fail("'git' was not found on PATH");
	};

	auto result = dalang::git::check_available(runner);
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("git is required"));
}

// ============================================================================
// Generator with a fake git
// ============================================================================

TEST(GeneratorTest, CreatesRenamedProject) {
	TempDir dir;
	std::vector<std::string> progress;
	dalang::Generator generator(fake_git(), [&](std::string_view line) { progress.emplace_back(line); });
	const auto dest = dir.path() / "my_app";

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_TRUE(result) << result.error().message;

	EXPECT_THAT(subcommands(generator.runner()), ElementsAre("--version", "clone", "init"));
	EXPECT_FALSE(fs::exists(dest / ".git"));
	EXPECT_EQ(read_file(dest / "CMakeLists.txt"), "project(\n  my_app\n)\n# my_app\n");
	EXPECT_THAT(read_file(dest / "README.md"), HasSubstr("# my_app"));
	EXPECT_THAT(read_file(dest / "README.md"), HasSubstr("`sample` template"));
	EXPECT_THAT(progress, Contains(HasSubstr("skipped not_in_template.txt")));
}

TEST(GeneratorTest, SkipsGitInitWhenAsked) {
	TempDir dir;
	dalang::Generator generator(fake_git());
	auto options = options_for(dir.path() / "my_app");
	options.init_git = false;

	ASSERT_TRUE(generator.generate(sample_template(), options));
	EXPECT_THAT(subcommands(generator.runner()), ElementsAre("--version", "clone"));
}

TEST(GeneratorTest, AcceptsExistingEmptyDirectory) {
	TempDir dir;
	dalang::Generator generator(fake_git());
	const auto dest = dir.path() / "empty";
	fs::create_directories(dest);

	EXPECT_TRUE(generator.generate(sample_template(), options_for(dest)));
}

TEST(GeneratorTest, RefusesNonEmptyDestinationWithoutRunningGit) {
	TempDir dir;
	dalang::Generator generator(fake_git());
	const auto dest = dir.path() / "taken";
	write_file(dest / "keep.txt", "mine");

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("not empty"));
	EXPECT_TRUE(generator.runner().calls.empty());
	EXPECT_EQ(read_file(dest / "keep.txt"), "mine");
}

TEST(GeneratorTest, RefusesFileAsDestination) {
	TempDir dir;
	dalang::Generator generator(fake_git());
	const auto dest = dir.path() / "file.txt";
	write_file(dest, "x");

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("not a directory"));
}

TEST(GeneratorTest, RejectsInvalidProjectNameWithoutRunningGit) {
	TempDir dir;
	dalang::Generator generator(fake_git());

	auto result = generator.generate(sample_template(), options_for(dir.path() / "x", "my app"));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("invalid project name"));
	EXPECT_TRUE(generator.runner().calls.empty());
}

TEST(GeneratorTest, StopsWhenGitIsMissing) {
	TempDir dir;
	dalang::Generator generator(fake_git("--version"));

	ASSERT_FALSE(generator.generate(sample_template(), options_for(dir.path() / "x")));
	EXPECT_THAT(subcommands(generator.runner()), ElementsAre("--version"));
}

TEST(GeneratorTest, RemovesCreatedDirectoryWhenInitFails) {
	TempDir dir;
	dalang::Generator generator(fake_git("init"));
	const auto dest = dir.path() / "my_app";

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("git init failed"));
	EXPECT_FALSE(fs::exists(dest));
}

TEST(GeneratorTest, EmptiesButKeepsExistingDirectoryWhenCloneFails) {
	TempDir dir;
	dalang::Generator generator(fake_git("clone"));
	const auto dest = dir.path() / "empty";
	fs::create_directories(dest);

	ASSERT_FALSE(generator.generate(sample_template(), options_for(dest)));
	EXPECT_TRUE(fs::exists(dest));
	EXPECT_TRUE(fs::is_empty(dest));
}

// ============================================================================
// Generator with real git against a local repository
// ============================================================================

namespace {

/// file:// URL for a local path. git ignores --depth for plain local paths.
std::string file_url(const fs::path &path) {
	const auto generic = path.generic_u8string();
	const std::string text(generic.begin(), generic.end());
	return text.starts_with('/') ? "file://" + text : "file:///" + text; // Windows: file:///C:/...
}

class RealGitTest : public ::testing::Test {
  protected:
	void SetUp() override {
		if (auto version = m_runner.run({.args = {"git", "--version"}, .env = {}}); !version) {
			GTEST_SKIP() << "git is not installed";
		}
		write_file(m_source / "CMakeLists.txt", "project(old_name)\n");
		write_file(m_source / "README.md", "template readme\n");
		git({"init", "--quiet"});
		commit("v1");
		git({"tag", "v1"});
		write_file(m_source / "CMakeLists.txt", "project(old_name)\n# v2\n");
		commit("v2");
	}

	void git(std::vector<std::string> args) {
		args.insert(args.begin(), {"git", "-C", dalang::to_utf8(m_source)});
		auto result = m_runner.run({.args = std::move(args), .env = {}});
		ASSERT_TRUE(result) << result.error().message;
		ASSERT_EQ(result->exit_code, 0) << result->output;
	}

	void commit(const std::string &message) {
		git({"add", "."});
		git({"-c", "user.name=dalang", "-c", "user.email=dalang@example.com", "commit", "--quiet", "-m", message});
	}

	TempDir m_dir;
	fs::path m_source = m_dir.path() / "template";
	dalang::ReprocRunner m_runner;
	dalang::Generator<> m_generator;
};

} // namespace

TEST_F(RealGitTest, GeneratesFromLocalRepository) {
	const auto dest = m_dir.path() / "out" / "my_app";
	auto result = m_generator.generate(sample_template(file_url(m_source)), options_for(dest));
	ASSERT_TRUE(result) << result.error().message;

	EXPECT_EQ(read_file(dest / "CMakeLists.txt"), "project(my_app)\n# v2\n");
	EXPECT_THAT(read_file(dest / "README.md"), HasSubstr("# my_app"));
	EXPECT_TRUE(fs::exists(dest / ".git" / "HEAD"));

	// A fresh repository: the template's commits are gone.
	auto log = m_runner.run({.args = {"git", "-C", dalang::to_utf8(dest), "log"}, .env = {}});
	ASSERT_TRUE(log);
	EXPECT_NE(log->exit_code, 0);
}

TEST_F(RealGitTest, ChecksOutRequestedTag) {
	const auto dest = m_dir.path() / "tagged";
	auto options = options_for(dest, "tagged");
	options.ref = "v1";
	options.init_git = false;

	auto result = m_generator.generate(sample_template(file_url(m_source)), options);
	ASSERT_TRUE(result) << result.error().message;
	EXPECT_EQ(read_file(dest / "CMakeLists.txt"), "project(tagged)\n");
	EXPECT_FALSE(fs::exists(dest / ".git"));
}

TEST_F(RealGitTest, UnknownRepositoryLeavesNothingBehind) {
	const auto dest = m_dir.path() / "never";
	auto result = m_generator.generate(sample_template(file_url(m_dir.path() / "missing")), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("git clone failed"));
	EXPECT_FALSE(fs::exists(dest));
}
