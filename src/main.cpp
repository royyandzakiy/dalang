// dalang command line: `new`, `list` and the interactive form.
#include "dalang/build_info.hpp"
#include "dalang/core.hpp"
#include "dalang/generator.hpp"

#include <CLI/CLI.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct GlobalArgs {
	bool verbose = false;
	bool quiet = false;
	std::string config;
};

struct NewArgs {
	std::string destination;
	std::string template_name = "min";
	std::string project_name; ///< empty: derived from the destination
	std::string ref;
	bool no_git_init = false;
	bool interactive = false;
};

void print_error(std::string_view message) {
	fmt::println(stderr, "error: {}", message);
}

/// Fills in `args` through a form. Returns false if the user cancels.
bool run_interactive(const dalang::TemplateRegistry &registry, NewArgs &args) {
	using namespace ftxui;

	const auto templates = registry.all();
	if (templates.empty()) {
		return false;
	}
	auto entries = templates | std::views::transform([](const dalang::Template &tpl) {
					   return fmt::format("{:<8} {}", tpl.name, tpl.description);
				   });
	std::vector<std::string> labels(entries.begin(), entries.end());
	const auto preselected = std::ranges::find(templates, args.template_name, &dalang::Template::name);
	int selected = preselected != templates.end() ? static_cast<int>(preselected - templates.begin()) : 0;

	auto screen = ScreenInteractive::TerminalOutput();
	bool confirmed = false;
	const auto confirm = [&] {
		confirmed = true;
		screen.Exit();
	};

	InputOption input_option;
	input_option.multiline = false;
	input_option.on_enter = confirm;
	input_option.placeholder = "my_project";
	auto destination = Input(&args.destination, input_option);
	input_option.placeholder = "same as the destination folder";
	auto project_name = Input(&args.project_name, input_option);
	auto template_list = Radiobox(&labels, &selected);
	auto create = Button("Create", confirm, ButtonOption::Ascii());
	auto cancel = Button("Cancel", screen.ExitLoopClosure(), ButtonOption::Ascii());

	auto form =
		Container::Vertical({template_list, destination, project_name, Container::Horizontal({create, cancel})});
	auto view = Renderer(form, [&] {
		return vbox({
				   text("dalang - new project") | bold,
				   separator(),
				   text("Template"),
				   template_list->Render(),
				   separator(),
				   hbox({text("Destination:  "), destination->Render()}),
				   hbox({text("Project name: "), project_name->Render()}),
				   separator(),
				   hbox({create->Render(), text(" "), cancel->Render()}),
				   text("Tab/arrows to move, Enter to confirm, Esc to cancel") | dim,
			   }) |
			   border;
	});
	view |= CatchEvent([&](const Event &event) {
		if (event == Event::Escape) {
			screen.Exit();
			return true;
		}
		return false;
	});

	screen.Loop(view);
	if (confirmed) {
		args.template_name = templates[static_cast<std::size_t>(selected)].name;
	}
	return confirmed;
}

int list_templates(const dalang::TemplateRegistry &registry) {
	const auto width = std::ranges::max(
		registry.all() | std::views::transform([](const dalang::Template &tpl) { return tpl.name.size(); }));
	for (const auto &tpl : registry.all()) {
		fmt::println("{:<{}}  {}", tpl.name, width, tpl.description);
		fmt::println("{:<{}}  {}", "", width, tpl.url);
	}

	if (const auto user_config = dalang::user_config_path(); !user_config.empty()) {
		std::error_code err;
		fmt::println("\nUser templates: {} ({})", dalang::to_utf8(user_config),
					 fs::exists(user_config, err) ? "loaded" : "not found");
	}
	return 0;
}

int new_project(const dalang::TemplateRegistry &registry, NewArgs args, const GlobalArgs &global) {
	if (args.interactive && !run_interactive(registry, args)) {
		fmt::println(stderr, "Cancelled");
		return 1;
	}
	if (args.destination.empty()) {
		print_error("missing destination folder, e.g. `dalang new my_project`");
		return 1;
	}
	const auto *tpl = registry.find(args.template_name);
	if (tpl == nullptr) {
		print_error(fmt::format("unknown template '{}' (available: {})", args.template_name,
								fmt::join(registry.names(), ", ")));
		return 1;
	}

	const auto destination = dalang::from_utf8(args.destination);
	const dalang::GenerateOptions options{
		.destination = destination,
		.project_name = args.project_name.empty() ? dalang::default_project_name(destination) : args.project_name,
		.ref = args.ref.empty() ? std::nullopt : std::optional(args.ref),
		.init_git = !args.no_git_init,
	};

	dalang::ProgressFn progress;
	if (!global.quiet) {
		progress = [](std::string_view line) { fmt::println("{}", line); };
	}
	dalang::Generator generator(dalang::ReprocRunner(global.verbose), std::move(progress));

	if (auto generated = generator.generate(*tpl, options); !generated) {
		print_error(generated.error().message);
		return 1;
	}
	if (!global.quiet) {
		fmt::println("\nCreated '{}' in {}\n\nNext:\n  cd {}", options.project_name, args.destination,
					 args.destination);
	}
	return 0;
}

int run(int argc, char **argv) {
	CLI::App app{"Create new projects from git templates.", "dalang"};
	argv = app.ensure_utf8(argv);
	app.set_version_flag("-V,--version", fmt::format("dalang {}", dalang::build_info::version));
	app.require_subcommand(1);
	app.fallthrough();
	app.get_formatter()->enable_footer_formatting(false);
	app.footer("Examples:\n"
			   "  dalang new my_app                  minimal C++ project in ./my_app\n"
			   "  dalang new fw -t zephyr -n blinky  Zephyr workspace named 'blinky'\n"
			   "  dalang new -i                      choose everything in a form\n"
			   "  dalang list                        show available templates");

	GlobalArgs global;
	auto *verbose = app.add_flag("-v,--verbose", global.verbose, "Print each git command and its output");
	app.add_flag("-q,--quiet", global.quiet, "Print errors only")->excludes(verbose);
	app.add_option("-c,--config", global.config, "Extra templates file, instead of the user config")
		->check(CLI::ExistingFile);

	NewArgs new_args;
	auto *new_cmd = app.add_subcommand("new", "Create a project from a template");
	new_cmd->add_option("destination", new_args.destination, "Folder to create (must not exist, or be empty)");
	new_cmd->add_option("-t,--template", new_args.template_name, "Template name, see `dalang list`")
		->capture_default_str();
	new_cmd->add_option("-n,--name", new_args.project_name, "Project name (default: destination folder name)");
	new_cmd->add_option("-r,--ref", new_args.ref, "Template branch or tag (default: its default branch)");
	new_cmd->add_flag("--no-git-init", new_args.no_git_init, "Skip creating a git repository");
	new_cmd->add_flag("-i,--interactive", new_args.interactive, "Pick template and destination in a form");

	auto *list_cmd = app.add_subcommand("list", "Show available templates");

	CLI11_PARSE(app, argc, argv);

	auto config = global.config.empty() ? std::nullopt : std::optional(dalang::from_utf8(global.config));
	auto registry = dalang::load_registry(config);
	if (!registry) {
		print_error(registry.error().message);
		return 1;
	}
	if (list_cmd->parsed()) {
		return list_templates(*registry);
	}
	return new_project(*registry, std::move(new_args), global);
}

} // namespace

auto main(int argc, char **argv) -> int {
	try {
		return run(argc, argv);
	} catch (const std::exception &e) {
		print_error(e.what());
		return 1;
	}
}
