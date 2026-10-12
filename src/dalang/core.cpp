#include "dalang/core.hpp"

#include "dalang/build_info.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>
#include <toml++/toml.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <ranges>
#include <sstream>
#include <system_error>

namespace dalang {

namespace fs = std::filesystem;

// ============================================================================
// Processes
// ============================================================================

Result<ProcessResult> ReprocRunner::run(const Command &command) const {
	if (command.args.empty()) {
		return fail("empty command");
	}
	const auto &program = command.args.front();
	if (m_echo) {
		fmt::println("$ {}", fmt::join(command.args, " "));
	}

	reproc::options options;
	options.redirect.in.type = reproc::redirect::discard;
	// Capture both so git's messages end up in dalang's errors, not interleaved on the console.
	options.redirect.out.type = reproc::redirect::pipe;
	options.redirect.err.type = reproc::redirect::pipe;
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
	if (m_echo && !output.empty()) {
		fmt::print("{}", output);
	}
	return ProcessResult{.exit_code = status, .output = std::move(output)};
}

// ============================================================================
// Templates
// ============================================================================

namespace {

Result<std::optional<Rename>> parse_rename(const toml::table &entry, std::string_view where) {
	const auto *node = entry.get("rename");
	if (node == nullptr) {
		return std::nullopt;
	}
	const auto *table = node->as_table();
	if (table == nullptr) {
		return fail(fmt::format("{}: 'rename' must be a table", where));
	}

	auto token = (*table)["token"].value<std::string>();
	if (!token || token->empty()) {
		return fail(fmt::format("{}: 'rename.token' must be a non-empty string", where));
	}
	const auto *files = (*table)["files"].as_array();
	if (files == nullptr || !files->is_homogeneous(toml::node_type::string)) {
		return fail(fmt::format("{}: 'rename.files' must be an array of strings", where));
	}

	Rename rename{.token = std::move(*token), .files = {}};
	for (const auto &file : *files) {
		rename.files.push_back(*file.value<std::string>());
	}
	return rename;
}

Result<Template> parse_template(const toml::node &node, std::string_view where) {
	const auto *entry = node.as_table();
	if (entry == nullptr) {
		return fail(fmt::format("{}: must be a table", where));
	}

	auto name = (*entry)["name"].value<std::string>();
	if (!name || name->empty()) {
		return fail(fmt::format("{}: 'name' must be a non-empty string", where));
	}
	if (name->find_first_of(" \t") != std::string::npos) {
		return fail(fmt::format("{}: name '{}' must not contain spaces", where, *name));
	}
	const auto named = fmt::format("{} ('{}')", where, *name);

	auto url = (*entry)["url"].value<std::string>();
	if (!url || url->empty()) {
		return fail(fmt::format("{}: 'url' must be a non-empty string", named));
	}

	return parse_rename(*entry, named).transform([&](std::optional<Rename> rename) {
		return Template{
			.name = std::move(*name),
			.url = std::move(*url),
			.description = (*entry)["description"].value_or(std::string{}),
			.rename = std::move(rename),
		};
	});
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
		return fail(
			fmt::format("{}:{}:{}: {}", source, err.source().begin.line, err.source().begin.column, err.description()));
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

	for (const auto &[index, entry] : std::views::enumerate(*entries)) {
		auto tpl = parse_template(entry, fmt::format("{}: template #{}", source, index + 1));
		if (!tpl) {
			return std::unexpected(std::move(tpl).error());
		}
		if (registry.find(tpl->name) != nullptr) {
			return fail(fmt::format("{}: template '{}' is defined twice", source, tpl->name));
		}
		registry.m_templates.push_back(std::move(*tpl));
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
	return parse(content.view(), to_utf8(path));
}

Result<TemplateRegistry> TemplateRegistry::builtin() {
	return parse(build_info::builtin_templates_toml, "built-in templates.toml");
}

void TemplateRegistry::merge(const TemplateRegistry &other) {
	for (const auto &tpl : other.m_templates) {
		if (auto existing = std::ranges::find(m_templates, tpl.name, &Template::name); existing != m_templates.end()) {
			*existing = tpl;
		} else {
			m_templates.push_back(tpl);
		}
	}
}

const Template *TemplateRegistry::find(std::string_view name) const {
	const auto found = std::ranges::find(m_templates, name, &Template::name);
	return found != m_templates.end() ? &*found : nullptr;
}

std::vector<std::string> TemplateRegistry::names() const {
	std::vector<std::string> result;
	std::ranges::copy(m_templates | std::views::transform(&Template::name), std::back_inserter(result));
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
	return base.empty() ? fs::path() : base / "dalang" / "templates.toml";
}

Result<TemplateRegistry> load_registry(const std::optional<fs::path> &config_path) {
	return TemplateRegistry::builtin().and_then([&](TemplateRegistry registry) -> Result<TemplateRegistry> {
		auto path = config_path.value_or(user_config_path());
		std::error_code err;
		if (!config_path && (path.empty() || !fs::exists(path, err))) {
			return registry;
		}
		return TemplateRegistry::load_file(path).transform([&](const TemplateRegistry &extra) {
			registry.merge(extra);
			return std::move(registry);
		});
	});
}

// ============================================================================
// Project files
// ============================================================================

Result<void> validate_project_name(std::string_view name) {
	const auto is_alpha = [](char chr) { return std::isalpha(static_cast<unsigned char>(chr)) != 0; };
	const auto is_valid = [&](char chr) {
		return is_alpha(chr) || std::isdigit(static_cast<unsigned char>(chr)) != 0 || chr == '_' || chr == '-';
	};
	if (name.empty() || !is_alpha(name.front()) || !std::ranges::all_of(name, is_valid)) {
		return fail(fmt::format("invalid project name '{}': use a letter followed by letters, digits, '_' or '-' "
								"(pass --name to choose one)",
								name));
	}
	return {};
}

std::string default_project_name(const fs::path &destination) {
	const auto path = fs::absolute(destination).lexically_normal();
	// A trailing separator leaves an empty filename, e.g. "my_project/".
	return to_utf8(path.has_filename() ? path.filename() : path.parent_path().filename());
}

Result<std::size_t> replace_in_file(const fs::path &file, std::string_view from, std::string_view to) {
	std::string content;
	{
		std::ifstream in(file, std::ios::binary);
		if (!in) {
			return fail(fmt::format("cannot read '{}'", to_utf8(file)));
		}
		std::ostringstream buffer;
		buffer << in.rdbuf();
		content = std::move(buffer).str();
	}

	std::size_t count = 0;
	for (auto pos = content.find(from); !from.empty() && pos != std::string::npos; pos = content.find(from, pos)) {
		content.replace(pos, from.size(), to);
		pos += to.size();
		++count;
	}
	if (count == 0) {
		return count;
	}

	std::ofstream out(file, std::ios::binary | std::ios::trunc);
	if (!out.write(content.data(), static_cast<std::streamsize>(content.size()))) {
		return fail(fmt::format("cannot write '{}'", to_utf8(file)));
	}
	return count;
}

void make_writable(const fs::path &dir) {
	std::error_code err;
	for (auto it = fs::recursive_directory_iterator(dir, err); !err && it != fs::recursive_directory_iterator();
		 it.increment(err)) {
		fs::permissions(it->path(), fs::perms::owner_write, fs::perm_options::add, err);
	}
}

} // namespace dalang
