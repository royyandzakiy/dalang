#include "dalang/git.hpp"
#include "mock_process_runner.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Pair;
using ::testing::Return;

TEST(GitTest, CloneIsShallowAndNeverPrompts) {
	MockProcessRunner runner;
	EXPECT_CALL(runner, run(AllOf(Field(&dalang::Command::args,
										ElementsAre("git", "clone", "--depth", "1", "https://x/t.git", "dest")),
								  Field(&dalang::Command::env, Contains(Pair("GIT_TERMINAL_PROMPT", "0"))))))
		.WillOnce(Return(exited(0)));

	dalang::Git git(runner);
	EXPECT_TRUE(git.clone("https://x/t.git", "dest", std::nullopt));
}

TEST(GitTest, ClonePassesRefAsBranch) {
	MockProcessRunner runner;
	EXPECT_CALL(runner, run(Field(&dalang::Command::args, ElementsAre("git", "clone", "--depth", "1", "--branch",
																	  "v1.2", "https://x/t.git", "dest"))))
		.WillOnce(Return(exited(0)));

	dalang::Git git(runner);
	EXPECT_TRUE(git.clone("https://x/t.git", "dest", "v1.2"));
}

TEST(GitTest, CloneFailureIncludesGitOutput) {
	MockProcessRunner runner;
	EXPECT_CALL(runner, run(_)).WillOnce(Return(exited(128, "fatal: repository not found\n")));

	dalang::Git git(runner);
	auto result = git.clone("https://x/missing.git", "dest", std::nullopt);
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("exit code 128"));
	EXPECT_THAT(result.error().message, HasSubstr("fatal: repository not found"));
}

TEST(GitTest, InitRunsInsideTheDirectory) {
	MockProcessRunner runner;
	EXPECT_CALL(runner, run(Field(&dalang::Command::args, ElementsAre("git", "-C", "dest", "init", "--quiet"))))
		.WillOnce(Return(exited(0)));

	dalang::Git git(runner);
	EXPECT_TRUE(git.init("dest"));
}

TEST(GitTest, MissingGitIsReported) {
	MockProcessRunner runner;
	EXPECT_CALL(runner, run(_)).WillOnce(Return(dalang::fail("'git' was not found on PATH")));

	dalang::Git git(runner);
	auto result = git.check_available();
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("git is required"));
}
