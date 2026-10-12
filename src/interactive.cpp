#include "interactive.hpp"

#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <vector>

namespace dalang {

std::optional<InteractiveChoice> run_interactive(const TemplateRegistry &registry, InteractiveChoice defaults) {
	using namespace ftxui;

	const auto templates = registry.all();
	if (templates.empty()) {
		return std::nullopt;
	}
	std::vector<std::string> entries;
	entries.reserve(templates.size());
	for (const auto &tpl : templates) {
		entries.push_back(fmt::format("{:<8} {}", tpl.name, tpl.description));
	}
	const auto preselected = std::ranges::find(templates, defaults.template_name, &Template::name);
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

	auto template_list = Radiobox(&entries, &selected);
	input_option.placeholder = "my_project";
	auto destination = Input(&defaults.destination, input_option);
	input_option.placeholder = "same as the destination folder";
	auto project_name = Input(&defaults.project_name, input_option);
	auto create = Button("Create", confirm, ButtonOption::Ascii());
	auto cancel = Button("Cancel", screen.ExitLoopClosure(), ButtonOption::Ascii());

	auto form = Container::Vertical({
		template_list,
		destination,
		project_name,
		Container::Horizontal({create, cancel}),
	});

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
	if (!confirmed) {
		return std::nullopt;
	}
	return InteractiveChoice{
		.template_name = templates[static_cast<std::size_t>(selected)].name,
		.destination = defaults.destination,
		.project_name = defaults.project_name,
	};
}

} // namespace dalang
