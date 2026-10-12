#pragma once

#include "dalang/templates.hpp"

#include <optional>
#include <string>

namespace dalang {

struct InteractiveChoice {
	std::string template_name;
	std::string destination;
	/// Empty to derive it from the destination.
	std::string project_name;
};

/// Full-screen form for picking a template and destination. Empty if the user cancels.
std::optional<InteractiveChoice> run_interactive(const TemplateRegistry &registry, InteractiveChoice defaults);

} // namespace dalang
