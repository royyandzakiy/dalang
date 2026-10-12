#pragma once

#include "dalang/process.hpp"

#include <gmock/gmock.h>

#include <string>
#include <utility>

class MockProcessRunner : public dalang::IProcessRunner {
  public:
	MOCK_METHOD(dalang::Result<dalang::ProcessResult>, run, (const dalang::Command &command), (override));
};

inline dalang::Result<dalang::ProcessResult> exited(int code, std::string output = {}) {
	return dalang::ProcessResult{.exit_code = code, .output = std::move(output)};
}
