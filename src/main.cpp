#include <CLI/CLI.hpp>
#include <array>
#include <cpp-subprocess/subprocess.hpp>
#include <cstdio>
#include <filesystem>
#include <fmt/base.h>
#include <fmt/color.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace sp = subprocess;
namespace fs = std::filesystem;

constexpr std::array<std::pair<std::string_view, std::string_view>, 4> template_type_map{{
	{"full", "https://github.com/royyandzakiy/cpp-project-template"},
	{"min", "https://github.com/royyandzakiy/cpp-project-template-min"},
	{"zephyr", "https://github.com/royyandzakiy/zephyr-project-template"},
	{"idf", "https://github.com/royyandzakiy/esp-idf-project-template"},
	{"qml", "https://github.com/royyandzakiy/qml-project-template"},
}};

std::string select_cpp_standard() {
	using namespace ftxui;
	auto screen = ScreenInteractive::FitComponent();
	std::vector<std::string> std_opt = {"C++11", "C++17", "C++20", "C++23"};
	int std_selected = 0;

	MenuOption option;
	option.on_enter = screen.ExitLoopClosure();

	auto menu = Menu(&std_opt, &std_selected, option);

	auto renderer = Renderer(menu, [&] {
		return vbox({
			text("Choose C++ standard:"),
			menu->Render(),
		});
	});

	screen.Loop(renderer);

	return std_opt.at(std_selected);
}

/// Clone `url` into `dest`. Returns the process exit code (0 on success).
int clone_template(const std::string &url, const fs::path &dest) {
	auto p = sp::Popen({"git", "clone", url, dest.string()}, sp::output{sp::PIPE}, sp::error{sp::PIPE});
	auto err = p.wait();
	if (err != 0) {
		fmt::println(stderr, "Git clone error code: {}", err);
	}
	return err;
}

/// Remove a single file, logging if it was missing or failed.
void remove_file(const fs::path &path) {
	std::error_code err;
	if (!fs::remove(path, err) && !err) {
		fmt::println("{} does not exist", path.filename().string());
	} else if (err) {
		fmt::println(stderr, "Failed to remove {}: {}", path.string(), err.message());
	}
}

/// Recursively remove a directory, logging if it was missing or failed.
void remove_dir(const fs::path &path) {
	std::error_code err;
	if (!fs::remove_all(path, err) && !err) {
		fmt::println("{} folder does not exist", path.filename().string());
	} else if (err) {
		fmt::println(stderr, "Failed to remove {}: {}", path.string(), err.message());
	}
}

/// Remove the cloned template's metadata (.git, README.md) and re-init git.
/// Returns the process exit code of `git init` (0 on success).
int reset_template_repo(const fs::path &dest) {
	remove_file(dest / "README.md");
	remove_dir(dest / ".git");

	auto current_path = fs::current_path();
	fs::current_path(dest);

	auto p = sp::Popen({"git", "init"}, sp::output{sp::PIPE}, sp::error{sp::PIPE});
	auto err = p.wait();

	fs::current_path(current_path);
	if (err != 0) {
		fmt::println(stderr, "Git init error code: {}", err);
	}
	return err;
}

/// Returns the URL for `key`, or an empty string_view if not found.
std::string_view find_template_url(std::string_view key) {
	for (const auto &[k, v] : template_type_map)
		if (k == key)
			return v;
	return {};
}

/// Clone `url` into `dest`. Returns the process exit code (0 on success).
int clone_template(std::string_view url, const fs::path &dest) {
	auto p = sp::Popen({"git", "clone", std::string(url), dest.string()}, sp::output{sp::PIPE}, sp::error{sp::PIPE});
	auto err = p.wait();
	if (err != 0) {
		fmt::println(stderr, "Git clone error code: {}", err);
	}
	return err;
}

auto main(int argc, char **argv) -> int {
	std::string selected_template_type{};
	fs::path selected_dest_folder{};

	// template type list
	std::vector<std::string_view> template_types;
	template_types.reserve(template_type_map.size());
	for (const auto &[k, _] : template_type_map)
		template_types.push_back(k);

	// parse CLI options
	CLI::App app{"Cecep C++ Project Generator"};
	argv = app.ensure_utf8(argv);
	app.add_option("dest,-d,--dest", selected_dest_folder, "The folder destination")
		->default_val("my_project")
		->capture_default_str();
	app.add_option("type,-t,--type", selected_template_type, "The type of template")
		->default_val("min")
		->check(CLI::IsMember(template_types)) // Validate input against template_type_map keys
		->capture_default_str();

	app.parse(argc, argv);

	const auto url = find_template_url(selected_template_type);
	if (url.empty()) {
		fmt::println(stderr, "Template '{}' not found", selected_template_type);
		return 1;
	}

	fmt::println("Generating project from template: {} -> {}", selected_template_type, url);
	fmt::println("Destination: {}", selected_dest_folder.string());

	// menu: show cpp standard
	// auto cpp_std = select_cpp_standard();
	// fmt::println("std: {}", cpp_std);

	// run git clone, change folder name
	if (auto err = clone_template(url, selected_dest_folder); err != 0) {
		return err;
	}

	// delete .git & readme, git init
	if (auto err = reset_template_repo(selected_dest_folder); err != 0) {
		return err;
	}

	fmt::println("Project generation successful!", selected_dest_folder.string());

	return 0;
}
