#include "dalang/templates.hpp"

#include "dalang/paths.hpp"

#include "dalang/build_info.hpp"

#include <fmt/format.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>

namespace dalang {

namespace fs = std::filesystem;

namespace {

std::optional<std::string> get_string(const toml::table &table, std::string_view key) {
	return table[key].value<std::string>();
}

Result<std::optional<Rename>> parse_rename(const toml::table &entry, std::string_view where) {
	const auto *node = entry.get("rename");
	if (node == nullptr) {
		return std::optional<Rename>{};
	}
	const auto *table = node->as_table();
	if (table == nullptr) {
		return fail(fmt::format("{}: 'rename' must be a table", where));
	}
	Rename rename;
	auto token = get_string(*table, "token");
	if (!token || token->empty()) {
		return fail(fmt::format("{}: 'rename.token' must be a non-empty string", where));
	}
	rename.token = std::move(*token);

	const auto *files = (*table)["files"].as_array();
	if (files == nullptr) {
		return fail(fmt::format("{}: 'rename.files' must be an array of strings", where));
	}
	for (const auto &file : *files) {
		auto value = file.value<std::string>();
		if (!value) {
			return fail(fmt::format("{}: 'rename.files' must be an array of strings", where));
		}
		rename.files.push_back(std::move(*value));
	}
	return std::optional<Rename>{std::move(rename)};
}

Result<Template> parse_template(const toml::node &node, std::string_view source, std::size_t index) {
	const auto where = fmt::format("{}: template #{}", source, index + 1);
	const auto *entry = node.as_table();
	if (entry == nullptr) {
		return fail(fmt::format("{}: must be a table", where));
	}

	Template tpl;
	auto name = get_string(*entry, "name");
	if (!name || name->empty()) {
		return fail(fmt::format("{}: 'name' must be a non-empty string", where));
	}
	if (name->find_first_of(" \t") != std::string::npos) {
		return fail(fmt::format("{}: name '{}' must not contain spaces", where, *name));
	}
	tpl.name = std::move(*name);

	auto url = get_string(*entry, "url");
	if (!url || url->empty()) {
		return fail(fmt::format("{} ('{}'): 'url' must be a non-empty string", where, tpl.name));
	}
	tpl.url = std::move(*url);
	tpl.description = get_string(*entry, "description").value_or("");

	auto rename = parse_rename(*entry, fmt::format("{} ('{}')", where, tpl.name));
	if (!rename) {
		return std::unexpected(rename.error());
	}
	tpl.rename = std::move(*rename);
	return tpl;
}

/// Reads an environment variable as a path, empty if unset.
fs::path env_path([[maybe_unused]] const char *name, [[maybe_unused]] const wchar_t *wide_name) {
#ifdef _WIN32
	wchar_t *raw = nullptr;
	std::size_t len = 0;
	if (_wdupenv_s(&raw, &len, wide_name) != 0 || raw == nullptr) {
		return {};
	}
	const std::unique_ptr<wchar_t, decltype(&std::free)> value(raw, &std::free);
	return {value.get()};
#else
	const char *value = std::getenv(name); // NOLINT(concurrency-mt-unsafe): read once at startup
	return value != nullptr ? fs::path(value) : fs::path();
#endif
}

} // namespace

Result<TemplateRegistry> TemplateRegistry::parse(std::string_view toml, std::string_view source) {
	auto parsed = toml::parse(toml, source);
	if (!parsed) {
		const auto &err = parsed.error();
		return fail(fmt::format("{}:{}:{}: {}", source, err.source().begin.line, err.source().begin.column,
								err.description()));
	}

	TemplateRegistry registry;
	const auto *node = parsed.table().get("template");
	if (node == nullptr) {
		return registry;
	}
	const auto *entries = node->as_array();
	if (entries == nullptr) {
		return fail(fmt::format("{}: 'template' must be an array of tables ([[template]])", source));
	}
	for (std::size_t i = 0; i < entries->size(); ++i) {
		auto tpl = parse_template(*entries->get(i), source, i);
		if (!tpl) {
			return std::unexpected(tpl.error());
		}
		if (registry.find(tpl->name) != nullptr) {
			return fail(fmt::format("{}: template '{}' is defined twice", source, tpl->name));
		}
		registry.templates_.push_back(std::move(*tpl));
	}
	return registry;
}

Result<TemplateRegistry> TemplateRegistry::load_file(const fs::path &path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		return fail(fmt::format("cannot read config file '{}'", to_utf8(path)));
	}
	std::ostringstream content;
	content << file.rdbuf();
	return parse(content.str(), to_utf8(path));
}

Result<TemplateRegistry> TemplateRegistry::builtin() {
	return parse(build_info::builtin_templates_toml, "built-in templates.toml");
}

void TemplateRegistry::merge(const TemplateRegistry &other) {
	for (const auto &tpl : other.templates_) {
		auto existing = std::ranges::find(templates_, tpl.name, &Template::name);
		if (existing != templates_.end()) {
			*existing = tpl;
		} else {
			templates_.push_back(tpl);
		}
	}
}

const Template *TemplateRegistry::find(std::string_view name) const {
	auto found = std::ranges::find(templates_, name, &Template::name);
	return found != templates_.end() ? &*found : nullptr;
}

std::vector<std::string> TemplateRegistry::names() const {
	std::vector<std::string> result;
	result.reserve(templates_.size());
	for (const auto &tpl : templates_) {
		result.push_back(tpl.name);
	}
	return result;
}

fs::path user_config_path() {
#ifdef _WIN32
	auto base = env_path("APPDATA", L"APPDATA");
#else
	auto base = env_path("XDG_CONFIG_HOME", nullptr);
	if (base.empty()) {
		if (auto home = env_path("HOME", nullptr); !home.empty()) {
			base = home / ".config";
		}
	}
#endif
	if (base.empty()) {
		return {};
	}
	return base / "dalang" / "templates.toml";
}

Result<TemplateRegistry> load_registry(const std::optional<fs::path> &config_path) {
	auto registry = TemplateRegistry::builtin();
	if (!registry) {
		return registry;
	}

	fs::path path;
	if (config_path) {
		path = *config_path;
	} else {
		path = user_config_path();
		std::error_code err;
		if (path.empty() || !fs::exists(path, err)) {
			return registry;
		}
	}

	auto extra = TemplateRegistry::load_file(path);
	if (!extra) {
		return extra;
	}
	registry->merge(*extra);
	return registry;
}

} // namespace dalang
