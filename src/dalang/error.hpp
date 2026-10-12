#pragma once

#include <expected>
#include <string>
#include <utility>

namespace dalang {

struct Error {
	std::string message;
};

template <typename T = void>
using Result = std::expected<T, Error>;

inline std::unexpected<Error> fail(std::string message) {
	return std::unexpected<Error>(Error{std::move(message)});
}

} // namespace dalang
