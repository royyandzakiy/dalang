#include "dalang/generator.hpp"
#include "mock_process_runner.hpp"
#include "temp_dir.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::Not;

namespace {

dalang::Template sample_template() {
	return dalang::Template{
		.name = "sample",
		.url = "https://example.com/sample.git",
		.description = "",
		.rename = dalang::Rename{.token = "old_name", .files = {"CMakeLists.txt", "not_in_template.txt"}},
	};
}

/// Fakes git: `clone` writes a small template tree at the destination; every command can be
/// made to fail by name.
class GeneratorTest : public ::testing::Test {
  protected:
	void SetUp() override {
		ON_CALL(runner, run(_)).WillByDefault([this](const dalang::Command &command) {
			const auto &args = command.args;
			const auto &subcommand = args.at(1) == "-C" ? args.at(3) : args.at(1);
			calls.push_back(subcommand);
			if (subcommand == failing_command) {
				return exited(1, "boom");
			}
			if (args.at(1) == "clone") {
				const fs::path dest = args.back();
				write_file(dest / ".git" / "HEAD", "ref: refs/heads/main\n");
				write_file(dest / "README.md", "# Template readme\n");
				write_file(dest / "CMakeLists.txt", "project(\n  old_name\n)\n# old_name\n");
			}
			return exited(0);
		});
	}

	dalang::GenerateOptions options_for(const fs::path &dest) const {
		return {.destination = dest, .project_name = "my_app", .ref = std::nullopt, .init_git = true};
	}

	TempDir dir;
	::testing::NiceMock<MockProcessRunner> runner;
	dalang::Git git{runner};
	std::vector<std::string> calls;
	std::string failing_command;
};

} // namespace

TEST_F(GeneratorTest, CreatesRenamedProject) {
	std::vector<std::string> progress;
	dalang::Generator generator(git, [&](std::string_view line) { progress.emplace_back(line); });
	const auto dest = dir.path() / "my_app";

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_TRUE(result) << result.error().message;

	EXPECT_THAT(calls, ElementsAre("--version", "clone", "init"));
	EXPECT_FALSE(fs::exists(dest / ".git"));
	EXPECT_EQ(read_file(dest / "CMakeLists.txt"), "project(\n  my_app\n)\n# my_app\n");
	EXPECT_THAT(read_file(dest / "README.md"), HasSubstr("# my_app"));
	EXPECT_THAT(read_file(dest / "README.md"), HasSubstr("`sample` template"));
	EXPECT_THAT(progress, ::testing::Contains(HasSubstr("skipped not_in_template.txt")));
}

TEST_F(GeneratorTest, SkipsGitInitWhenAsked) {
	dalang::Generator generator(git);
	auto options = options_for(dir.path() / "my_app");
	options.init_git = false;

	ASSERT_TRUE(generator.generate(sample_template(), options));
	EXPECT_THAT(calls, ElementsAre("--version", "clone"));
}

TEST_F(GeneratorTest, AcceptsExistingEmptyDirectory) {
	dalang::Generator generator(git);
	const auto dest = dir.path() / "empty";
	fs::create_directories(dest);

	EXPECT_TRUE(generator.generate(sample_template(), options_for(dest)));
}

TEST_F(GeneratorTest, RefusesNonEmptyDestination) {
	dalang::Generator generator(git);
	const auto dest = dir.path() / "taken";
	write_file(dest / "keep.txt", "mine");

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("not empty"));
	EXPECT_TRUE(calls.empty());
	EXPECT_EQ(read_file(dest / "keep.txt"), "mine");
}

TEST_F(GeneratorTest, RefusesFileAsDestination) {
	dalang::Generator generator(git);
	const auto dest = dir.path() / "file.txt";
	write_file(dest, "x");

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("not a directory"));
}

TEST_F(GeneratorTest, RejectsInvalidProjectNameBeforeRunningGit) {
	dalang::Generator generator(git);
	auto options = options_for(dir.path() / "x");
	options.project_name = "my app";

	auto result = generator.generate(sample_template(), options);
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("invalid project name"));
	EXPECT_TRUE(calls.empty());
}

TEST_F(GeneratorTest, StopsWhenGitIsMissing) {
	failing_command = "--version";
	dalang::Generator generator(git);

	auto result = generator.generate(sample_template(), options_for(dir.path() / "x"));
	ASSERT_FALSE(result);
	EXPECT_THAT(calls, ElementsAre("--version"));
}

TEST_F(GeneratorTest, RemovesCreatedDirectoryWhenInitFails) {
	failing_command = "init";
	dalang::Generator generator(git);
	const auto dest = dir.path() / "my_app";

	auto result = generator.generate(sample_template(), options_for(dest));
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("git init failed"));
	EXPECT_FALSE(fs::exists(dest));
}

TEST_F(GeneratorTest, EmptiesButKeepsExistingDirectoryWhenCloneFails) {
	failing_command = "clone";
	dalang::Generator generator(git);
	const auto dest = dir.path() / "empty";
	fs::create_directories(dest);

	ASSERT_FALSE(generator.generate(sample_template(), options_for(dest)));
	EXPECT_TRUE(fs::exists(dest));
	EXPECT_TRUE(fs::is_empty(dest));
}

TEST(ProjectNameTest, Validation) {
	EXPECT_TRUE(dalang::validate_project_name("my_app"));
	EXPECT_TRUE(dalang::validate_project_name("App-2"));
	EXPECT_FALSE(dalang::validate_project_name(""));
	EXPECT_FALSE(dalang::validate_project_name("2app"));
	EXPECT_FALSE(dalang::validate_project_name("_app"));
	EXPECT_FALSE(dalang::validate_project_name("my app"));
	EXPECT_FALSE(dalang::validate_project_name("my.app"));
}

TEST(ProjectNameTest, DefaultsToLastPathComponent) {
	EXPECT_EQ(dalang::default_project_name(fs::path("projects") / "my_app"), "my_app");
	EXPECT_EQ(dalang::default_project_name("my_app/"), "my_app");
}

TEST(ReplaceInFileTest, ReplacesEveryOccurrence) {
	TempDir dir;
	const auto file = dir.path() / "f.txt";
	write_file(file, "aXa aa");

	auto count = dalang::replace_in_file(file, "a", "aa");
	ASSERT_TRUE(count);
	EXPECT_EQ(*count, 4U);
	EXPECT_EQ(read_file(file), "aaXaa aaaa");
}

TEST(ReplaceInFileTest, MissingFileIsAnError) {
	TempDir dir;
	auto count = dalang::replace_in_file(dir.path() / "missing.txt", "a", "b");
	ASSERT_FALSE(count);
	EXPECT_THAT(count.error().message, Not(HasSubstr("write")));
}
