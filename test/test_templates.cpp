#include "dalang/templates.hpp"
#include "temp_dir.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using dalang::TemplateRegistry;
using ::testing::ElementsAre;
using ::testing::HasSubstr;

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

TEST(TemplateRegistryTest, RejectsMissingUrl) {
	auto registry = TemplateRegistry::parse("[[template]]\nname = \"x\"\n", "test.toml");
	ASSERT_FALSE(registry);
	EXPECT_THAT(registry.error().message, HasSubstr("'url'"));
}

TEST(TemplateRegistryTest, RejectsNameWithSpaces) {
	auto registry = TemplateRegistry::parse("[[template]]\nname = \"a b\"\nurl = \"u\"\n", "test.toml");
	ASSERT_FALSE(registry);
	EXPECT_THAT(registry.error().message, HasSubstr("spaces"));
}

TEST(TemplateRegistryTest, RejectsDuplicateNames) {
	auto registry = TemplateRegistry::parse(R"(
		[[template]]
		name = "x"
		url = "a"
		[[template]]
		name = "x"
		url = "b"
	)",
											"test.toml");
	ASSERT_FALSE(registry);
	EXPECT_THAT(registry.error().message, HasSubstr("defined twice"));
}

TEST(TemplateRegistryTest, RejectsBadRename) {
	auto registry = TemplateRegistry::parse(
		"[[template]]\nname = \"x\"\nurl = \"u\"\nrename = { token = \"t\", files = \"CMakeLists.txt\" }\n",
		"test.toml");
	ASSERT_FALSE(registry);
	EXPECT_THAT(registry.error().message, HasSubstr("rename.files"));
}

TEST(TemplateRegistryTest, ReportsSyntaxErrorLocation) {
	auto registry = TemplateRegistry::parse("[[template]]\nname = \n", "broken.toml");
	ASSERT_FALSE(registry);
	EXPECT_THAT(registry.error().message, HasSubstr("broken.toml:2:"));
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
