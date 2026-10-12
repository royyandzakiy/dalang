#include "dalang/core.hpp"
#include "test_utils.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace fs = std::filesystem;
using dalang::TemplateRegistry;
using ::testing::ElementsAre;
using ::testing::HasSubstr;

// ============================================================================
// TemplateRegistry
// ============================================================================

TEST(TemplateRegistryTest, BuiltinTemplatesParse) {
	auto registry = TemplateRegistry::builtin();
	ASSERT_TRUE(registry) << registry.error().message;
	EXPECT_THAT(registry->names(), ElementsAre("min", "full", "qml", "zephyr", "idf"));

	const auto *min = registry->find("min");
	ASSERT_NE(min, nullptr);
	ASSERT_TRUE(min->rename);
	EXPECT_EQ(min->rename->token, "cpp_project_template");
	EXPECT_FALSE(registry->find("zephyr")->rename);
}

TEST(TemplateRegistryTest, ParsesAllFields) {
	auto registry = TemplateRegistry::parse(R"(
		[[template]]
		name = "lib"
		url = "https://example.com/lib.git"
		description = "A library"
		rename = { token = "old_name", files = ["CMakeLists.txt", "src/main.cpp"] }
	)",
											"test.toml");
	ASSERT_TRUE(registry) << registry.error().message;
	const auto *tpl = registry->find("lib");
	ASSERT_NE(tpl, nullptr);
	EXPECT_EQ(tpl->url, "https://example.com/lib.git");
	EXPECT_EQ(tpl->description, "A library");
	ASSERT_TRUE(tpl->rename);
	EXPECT_EQ(tpl->rename->token, "old_name");
	EXPECT_THAT(tpl->rename->files, ElementsAre("CMakeLists.txt", "src/main.cpp"));
}

TEST(TemplateRegistryTest, EmptyFileHasNoTemplates) {
	auto registry = TemplateRegistry::parse("", "empty.toml");
	ASSERT_TRUE(registry);
	EXPECT_TRUE(registry->all().empty());
	EXPECT_EQ(registry->find("min"), nullptr);
}

TEST(TemplateRegistryTest, RejectsInvalidEntries) {
	const auto error_for = [](std::string_view toml) {
		auto registry = TemplateRegistry::parse(toml, "test.toml");
		return registry ? std::string("parsed") : registry.error().message;
	};
	EXPECT_THAT(error_for("[[template]]\nname = \"x\"\n"), HasSubstr("'url'"));
	EXPECT_THAT(error_for("[[template]]\nname = \"a b\"\nurl = \"u\"\n"), HasSubstr("spaces"));
	EXPECT_THAT(error_for("[[template]]\nname = \"x\"\nurl = \"a\"\n[[template]]\nname = \"x\"\nurl = \"b\"\n"),
				HasSubstr("defined twice"));
	EXPECT_THAT(error_for("[[template]]\nname = \"x\"\nurl = \"u\"\nrename = { token = \"t\", files = \"f\" }\n"),
				HasSubstr("rename.files"));
	EXPECT_THAT(error_for("template = 1\n"), HasSubstr("array of tables"));
	EXPECT_THAT(error_for("[[template]]\nname = \n"), HasSubstr("test.toml:2:"));
}

TEST(TemplateRegistryTest, MergeReplacesInPlaceAndAppends) {
	auto base = TemplateRegistry::parse("[[template]]\nname = \"a\"\nurl = \"1\"\n"
										"[[template]]\nname = \"b\"\nurl = \"2\"\n",
										"base");
	auto extra = TemplateRegistry::parse("[[template]]\nname = \"c\"\nurl = \"3\"\n"
										 "[[template]]\nname = \"a\"\nurl = \"overridden\"\n",
										 "extra");
	ASSERT_TRUE(base && extra);
	base->merge(*extra);
	EXPECT_THAT(base->names(), ElementsAre("a", "b", "c"));
	EXPECT_EQ(base->find("a")->url, "overridden");
}

TEST(LoadRegistryTest, MergesExplicitConfigIntoBuiltins) {
	TempDir dir;
	const auto config = dir.path() / "templates.toml";
	write_file(config, "[[template]]\nname = \"mine\"\nurl = \"https://example.com/mine.git\"\n");

	auto registry = dalang::load_registry(config);
	ASSERT_TRUE(registry) << registry.error().message;
	EXPECT_NE(registry->find("min"), nullptr);
	EXPECT_NE(registry->find("mine"), nullptr);
}

TEST(LoadRegistryTest, MissingExplicitConfigIsAnError) {
	TempDir dir;
	auto registry = dalang::load_registry(dir.path() / "nope.toml");
	ASSERT_FALSE(registry);
	EXPECT_THAT(registry.error().message, HasSubstr("cannot read config file"));
}

TEST(LoadRegistryTest, UserConfigPathEndsInDalang) {
	const auto path = dalang::user_config_path();
	if (path.empty()) {
		GTEST_SKIP() << "no config base directory in this environment";
	}
	EXPECT_EQ(path.filename(), "templates.toml");
	EXPECT_EQ(path.parent_path().filename(), "dalang");
}

// ============================================================================
// Project files
// ============================================================================

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
	EXPECT_THAT(count.error().message, HasSubstr("cannot read"));
}

TEST(Utf8PathTest, RoundTripsNonAscii) {
	const std::string name = "proj\xC3\xA9t"; // "projét"
	EXPECT_EQ(dalang::to_utf8(dalang::from_utf8(name)), name);
}

// ============================================================================
// ReprocRunner
// ============================================================================

TEST(ReprocRunnerTest, CapturesOutputAndExitCode) {
	dalang::ReprocRunner runner;
	auto result = runner.run({.args = {"git", "--version"}, .env = {}});
	if (!result) {
		GTEST_SKIP() << "git is not installed";
	}
	EXPECT_EQ(result->exit_code, 0);
	EXPECT_THAT(result->output, HasSubstr("git version"));
}

TEST(ReprocRunnerTest, ReportsMissingProgram) {
	dalang::ReprocRunner runner;
	auto result = runner.run({.args = {"dalang-no-such-program"}, .env = {}});
	ASSERT_FALSE(result);
	EXPECT_THAT(result.error().message, HasSubstr("not found on PATH"));
}
