#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace dalang {

/// UTF-8 conversions for paths. path::string() and path(std::string) use the ANSI code page
/// on Windows, which mangles non-ASCII folder names.
inline std::string to_utf8(const std::filesystem::path &path) {
	const auto text = path.u8string();
	return {text.begin(), text.end()};
}

inline std::filesystem::path from_utf8(std::string_view text) {
	return {std::u8string(text.begin(), text.end())};
}

} // namespace dalang
