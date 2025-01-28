#include "dirscan.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <optional>

/*
 * Usage:
 *   ./my_grep_like_util <query> <directory> [--regex] [--ext .txt]
 *
 * Examples:
 *   ./my_grep_like_util "some_text" /path/to/search
 *   ./my_grep_like_util "^[A-Z]\\w+" /path/to/search --regex
 *   ./my_grep_like_util "needle" /path/to/search --ext .txt
 */
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] 
                  << " <query> <directory> [--regex] [--ext .xyz]\n";
        return 1;
    }

    std::string query = argv[1];
    std::filesystem::path directory = argv[2];
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        std::cerr << "Error: The specified path is not a directory or does not exist.\n";
        return 1;
    }

    bool use_regex = false;
    std::optional<std::string> extensionFilter;

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--regex") {
            use_regex = true;
        } else if (arg == "--ext" && i + 1 < argc) {
            extensionFilter = argv[++i]; // e.g. ".txt"
        }
    }

    searchInDirectory(query, directory, use_regex, extensionFilter);

    return 0;
}
