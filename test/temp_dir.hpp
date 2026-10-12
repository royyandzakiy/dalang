#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

/// A fresh directory under the system temp dir, deleted with its contents on destruction.
class TempDir {
  public:
	TempDir() {
		std::random_device random;
		path_ = std::filesystem::temp_directory_path() / ("dalang_test_" + std::to_string(random()));
		std::filesystem::create_directories(path_);
	}
	~TempDir() {
		namespace fs = std::filesystem;
		std::error_code err;
		// git marks its object files read-only, which blocks remove_all() on Windows.
		for (auto it = fs::recursive_directory_iterator(path_, err); !err && it != fs::recursive_directory_iterator();
			 it.increment(err)) {
			fs::permissions(it->path(), fs::perms::owner_write, fs::perm_options::add, err);
		}
		fs::remove_all(path_, err);
	}
	TempDir(const TempDir &) = delete;
	TempDir &operator=(const TempDir &) = delete;
	TempDir(TempDir &&) = delete;
	TempDir &operator=(TempDir &&) = delete;

	[[nodiscard]] const std::filesystem::path &path() const { return path_; }

  private:
	std::filesystem::path path_;
};

inline void write_file(const std::filesystem::path &file, std::string_view content) {
	std::filesystem::create_directories(file.parent_path());
	std::ofstream(file, std::ios::binary) << content;
}

inline std::string read_file(const std::filesystem::path &file) {
	std::ifstream in(file, std::ios::binary);
	std::ostringstream content;
	content << in.rdbuf();
	return content.str();
}
