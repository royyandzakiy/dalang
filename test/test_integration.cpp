#include "dalang/generator.hpp"
#include "dalang/paths.hpp"
#include "dalang/process.hpp"
#include "temp_dir.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace fs = std::filesystem;
using ::testing::HasSubstr;

namespace {

/// file:// URL for a local repository. git ignores --depth for plain local paths.
std::string file_url(const fs::path &path) {
	const auto generic = path.generic_u8string();
	std::string text(generic.begin(), generic.end());
	if (text.front() != '/') {
		text.insert(0, "/"); // Windows drive letter: file:///C:/...
	}
	return "file://" + text;
}

/// Runs real git against a throwaway local repository acting as the template.
class IntegrationTest : public ::testing::Test {
  protected:
	void SetUp() override {
		auto version = runner.run({.args = {"git", "--version"}, .env = {}});
		if (!version || version->exit_code != 0) {
			GTEST_SKIP() << "git is not installed";
		}
		write_file(source / "CMakeLists.txt", "project(tpl_name)\n");
		write_file(source / "README.md", "template readme\n");
		git({"init", "--quiet"});
		commit("v1");
		git({"tag", "v1"});
		write_file(source / "CMakeLists.txt", "project(tpl_name)\n# v2\n");
		commit("v2");
	}

	void git(std::vector<std::string> args) {
		args.insert(args.begin(), {"git", "-C", dalang::to_utf8(source)});
		auto result = runner.run({.args = std::move(args), .env = {}});
		ASSERT_TRUE(result) << result.error().message;
		ASSERT_EQ(result->exit_code, 0) << result->output;
	}

	void commit(const std::string &message) {
		git({"add", "."});
		git({"-c", "user.name=dalang", "-c", "user.email=dalang@example.com", "commit", "--quiet", "-m", message});
	}

	dalang::Template local_template() const {
		return {.name = "local",
				.url = file_url(source),
				.description = "",
				.rename = dalang::Rename{.token = "tpl_name", .files = {"CMakeLists.txt"}}};
	}

	TempDir dir;
	fs::path source = dir.path() / "template";
	dalang::ReprocRunner runner;
	dalang::Git real_git{runner};
	dalang::Generator generator{real_git};
};

} // namespace

TEST_F(IntegrationTest, GeneratesFromLocalRepository) {
	const auto dest = dir.path() / "out" / "my_app";
	auto result = generator.generate(local_template(),
									 {.destination = dest, .project_name = "my_app", .ref = std::nullopt, .init_git = true});
	ASSERT_TRUE(result) << result.error().message;

	EXPECT_EQ(read_file(dest / "CMakeLists.txt"), "project(my_app)\n# v2\n");
	EXPECT_THAT(read_file(dest / "README.md"), HasSubstr("# my_app"));
	EXPECT_TRUE(fs::exists(dest / ".git" / "HEAD"));

	// A fresh repository: the template's commits are gone.
	auto log = runner.run({.args = {"git", "-C", dalang::to_utf8(dest), "log"}, .env = {}});
	ASSERT_TRUE(log);
	EXPECT_NE(log->exit_code, 0);
}

TEST_F(IntegrationTest, ChecksOutRequestedTag) {
	const auto dest = dir.path() / "tagged";
	auto result = generator.generate(local_template(),
									 {.destination = dest, .project_name = "tagged", .ref = "v1", .init_git = false});
	ASSERT_TRUE(result) << result.error().message;

	EXPECT_EQ(read_file(dest / "CMakeLists.txt"), "project(tagged)\n");
	EXPECT_FALSE(fs::exists(dest / ".git"));
}

TEST_F(IntegrationTest, UnknownRepositoryLeavesNothingBehind) {
	auto tpl = local_template();
	tpl.url = file_url(dir.path() / "does_not_exist");
	const auto dest = dir.path() / "never";

	auto result = generator.generate(tpl, {.destination = dest, .project_name = "never", .ref = std::nullopt, .init_git = true});
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("git clone failed"));
	EXPECT_FALSE(fs::exists(dest));
}

TEST(ReprocRunnerTest, ReportsMissingProgram) {
	dalang::ReprocRunner runner;
	auto result = runner.run({.args = {"dalang-no-such-program"}, .env = {}});
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("not found on PATH"));
}
