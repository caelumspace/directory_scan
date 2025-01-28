#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "dirscan.h"

namespace fs = std::filesystem;

static void createSampleFile(const fs::path& path, const std::string& content) {
    std::ofstream ofs(path);
    ofs << content;
}

int main() {
    // 1. Create a temporary test directory and test files
    fs::path testDir = fs::temp_directory_path() / "test_files";
    fs::create_directories(testDir);

    std::cout << testDir;

    // File 1: Contains the word "needle"
    auto file1 = testDir / "file1.txt";
    createSampleFile(file1, "This is a test.\nWe have a needle here.\nEnd of file.\n");

    // File 2: No "needle"
    auto file2 = testDir / "file2.txt";
    createSampleFile(file2, "Some other text.\nNothing interesting.\n");

	searchInDirectory("needle", testDir, false, std::nullopt);

	// 3. Open search_results.txt and look for references
	std::ifstream results("search_results.txt");
	if (!results.is_open()) {
		std::cerr << "Error: Could not open search_results.txt\n";
		return 1;
	}

	bool foundNeedleInFile1 = false;
	bool foundNeedleInFile2 = false;

	std::string line;
	while (std::getline(results, line)) {
		// If line references file1.txt => foundNeedleInFile1 = true
		// If line references file2.txt => foundNeedleInFile2 = true
		if (line.find("file1.txt") != std::string::npos) {
			foundNeedleInFile1 = true;
		}
		if (line.find("file2.txt") != std::string::npos) {
			foundNeedleInFile2 = true;
		}
	}

	// Then assert the same as before
	assert(foundNeedleInFile1 && "Should have found 'needle' in file1.txt");
	assert(!foundNeedleInFile2 && "Should not have found 'needle' in file2.txt");

	// 4. Clean up
	/*fs::remove_all(testDir);
	fs::remove("search_results.txt");*/

	std::cout << "All tests passed!\n";
	return 0;
}
