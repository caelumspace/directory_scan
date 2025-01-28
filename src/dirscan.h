#ifndef GREP_H
#define GREP_H

#include <string>
#include <filesystem>
#include <vector>
#include <optional>
#include <regex>


/**
 * @brief Convert a wildcard pattern (e.g. "*.txt") to a std::regex string (e.g. "^.*\\.txt$")
 *
 * Supported wildcards:
 *   - `*` matches zero or more characters (except directory separators)
 *   - `?` matches exactly one character
 *
 * @param wildcard A string containing the wildcard expression.
 * @return A valid std::string that can be used to construct a std::regex.
 */
static std::string wildcardToRegex(const std::string& wildcard);

/**
 * @brief Helper function that checks if a given filename matches a wildcard (e.g. "*.txt").
 * @param filename The filename to test.
 * @param wildcardRegex A pre-built regex from wildcardToRegex().
 * @return true if it matches, false otherwise.
 */
static bool matchesWildcard(const std::string& filename, const std::regex& wildcardRegex) {
    return std::regex_match(filename, wildcardRegex);
}

static void searchInFile(const std::filesystem::path& filePath,
                         const std::string& query,
                         bool use_regex);


/**
 * @brief Recursively scans the specified directory, searching files for a query.
 * @param query Substring or regex query
 * @param directory Path of directory to search
 * @param use_regex If true, 'query' is interpreted as a regular expression
 * @param filePattern If present, a wildcard like "*.txt", "*.cpp", etc.
 */
void searchInDirectory(const std::string& query,
                       const std::filesystem::path& directory,
                       bool use_regex,
                       const std::optional<std::string>& filePattern);




#endif // GREP_H
