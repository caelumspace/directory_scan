#include <atomic>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>
#include "bounded_file_queue.h"
#include "dirscan.h"

struct StatusData {
	size_t filesScanned = 0;
	std::string currentFile;
	size_t fileHits = 0;
	size_t totalHits = 0;
	std::string lastError = "none";
};

static StatusData g_status;
static std::mutex g_statusMutex;
static std::mutex g_outputMutex;
static std::mutex g_resultsMutex;  // Protects g_resultsFile
static std::ofstream g_resultsFile;
static std::atomic<size_t> g_filesProcessed{ 0 };

// Clear screen & print table
static void printStatusTable()
{
	std::cout << "\033[2J\033[H"; // ANSI: clear and move cursor home
	std::cout
		<< "----------------------------------------------------\n"
		<< "| Files Scanned: " << g_status.filesScanned << "\n"
		<< "| Current File:  " << g_status.currentFile << "\n"
		<< "| Total hits:    " << g_status.totalHits << "\n"
		<< "|                                                  \n"
		<< "| Last Error:    " << g_status.lastError << "\n"
		<< "----------------------------------------------------\n";
	std::cout.flush();
}

static std::string sanitizeLine(const std::string& line)
{
	std::ostringstream oss;
	oss << std::noshowbase << std::hex; // Setup hex output without "0x"

	for (unsigned char c : line) {
		// If it's a normal printable ASCII character (excluding DEL = 127),
		// or space/tab, keep as-is:
		if ((c >= 32 && c < 127) || c == '\t') {
			oss << c;
		}
		else {
			// Otherwise, escape as \x?? with two hex digits
			oss << "\\x" << std::setw(2) << std::setfill('0') << (int)c;
		}
	}
	return oss.str();
}

/**
 * @brief Truncates a line around a match, and highlights the match.
 * @param line The line of text to process
 * @param query The query string to match
 * @param maxContext Maximum number of characters to include around the match
 * @return A new string with the match highlighted, and possibly truncated.
 */
static std::string truncateAndHighlightMatch(const std::string& line,
	const std::string& query,
	size_t maxContext = 160)
{
	// Find where the query first appears
	size_t pos = line.find(query);
	if (pos == std::string::npos) {
		// No match found in this line.
		// Optionally truncate line if it's very long,
		// but here let's just return it uncolored:
		if (line.size() > maxContext) {
			return line.substr(0, maxContext) + "...(truncated)";
		}
		return line;
	}

	// We have a match. Let's define how many chars to include around the match.
	// Example: we want 80 chars before, plus the match, plus 80 after = 160 total
	// or up to line boundaries.
	size_t contextRadius = maxContext / 2; // e.g. 80 if maxContext=160

	// Calculate start index of the snippet
	// Make sure we don't go beyond the beginning:
	size_t start = (pos > contextRadius) ? pos - contextRadius : 0;

	// Calculate the end index
	// We'll at least include the entire match, plus some context after.
	size_t end = pos + query.size() + contextRadius;
	if (end > line.size()) {
		end = line.size();
	}

	// Extract the snippet
	std::string snippet = line.substr(start, end - start);

	// For clarity, if we truncated from the left:
	bool truncatedLeft = (start > 0);
	// If we truncated from the right:
	bool truncatedRight = (end < line.size());

	// Insert the color codes into the snippet for the match
	// But note: 'pos' was relative to the entire line,
	// we need its position in the snippet:
	size_t snippetMatchPos = pos - start;

	// Insert reset code after the match
	snippet.insert(snippetMatchPos + query.size(), "\033[0m");
	// Insert red code before the match
	snippet.insert(snippetMatchPos, "\033[31m");

	// Append some indicator if truncated
	if (truncatedLeft) {
		snippet = "... " + snippet;
	}
	if (truncatedRight) {
		snippet += " ...";
	}

	return snippet;
}

/**
 * @brief Reads a file line by line, searching for a query (regex or substring).
 *        If matched, prints the result to stdout.
 */
static void searchInFile(const std::filesystem::path& filePath,
	const std::string& query,
	bool use_regex)
{
	// Attempt to open file
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs.is_open()) {
		std::lock_guard<std::mutex> lock(g_statusMutex);
		g_status.lastError = "Could not open: " + filePath.string();
		return;
	}

	// Prepare regex if needed
	std::regex pattern;
	if (use_regex) {
		try {
			pattern = std::regex(query);
		}
		catch (const std::regex_error&) {
			std::lock_guard<std::mutex> lock(g_statusMutex);
			g_status.lastError = "Invalid regex: " + query;
			return;
		}
	}

	// A local container for lines that matched
	struct MatchInfo {
		size_t lineNumber;
		std::string text;
	};
	std::vector<MatchInfo> matches;
	matches.reserve(100); // arbitrary

	std::string line;
	size_t lineNumber = 1;
	while (std::getline(ifs, line)) {
		bool found = false;
		if (use_regex) {
			if (std::regex_search(line, pattern)) {
				found = true;
			}
		}
		else {
			if (line.find(query) != std::string::npos) {
				found = true;
			}
		}

		if (found) {
			// Use our new function, with a 180-char window around the match
			std::string snippet = truncateAndHighlightMatch(line, query, 180);
			snippet = sanitizeLine(snippet);
			matches.push_back(MatchInfo{ lineNumber, std::move(snippet) });

			// Update status counters
			{
				std::lock_guard<std::mutex> lock(g_statusMutex);
				g_status.fileHits++;
				g_status.totalHits++;
			}
		}
		++lineNumber;
	}

	g_filesProcessed.fetch_add(1, std::memory_order_relaxed);

	{
		std::lock_guard<std::mutex> lock(g_statusMutex);
		g_status.filesScanned++; // done scanning this file
	}

	// If we found any matches, write them to the results file
	if (!matches.empty()) {
		std::lock_guard<std::mutex> outLock(g_resultsMutex);
		g_resultsFile << "Matches in file: " << filePath.string()
			<< " (" << matches.size() << " hits)\n";
		for (auto& m : matches) {
			g_resultsFile << "    Line " << m.lineNumber << ": "
				<< m.text << "\n";
		}
		g_resultsFile << "\n"; // extra blank line
		g_resultsFile.flush();
	}
}

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
	const std::optional<std::string>& filePattern)
{
	// Open results file (overwrite if it existed).
	{
		std::lock_guard<std::mutex> lock(g_resultsMutex);
		g_resultsFile.open("search_results.txt", std::ios::out | std::ios::trunc);
		if (!g_resultsFile.is_open()) {
			std::cerr << "Error: Could not open search_results.txt for writing.\n";
			return;
		}
	}

	// 1. Create a bounded queue with a capacity to handle concurrency without storing everything
	const size_t MAX_QUEUE_SIZE = 10000;  // tune as needed
	BoundedFileQueue fileQueue(MAX_QUEUE_SIZE);

	// Pre-build a regex for the wildcard pattern if specified
	std::optional<std::regex> wildcardRegex;
	if (filePattern.has_value()) {
		try {
			wildcardRegex = std::regex(wildcardToRegex(filePattern.value()),
				std::regex::icase); // 'icase' for case-insensitive, if desired
		}
		catch (const std::exception& e) {
			std::lock_guard<std::mutex> lock(g_outputMutex);
			std::cerr << "Error: Invalid file pattern: " << filePattern.value()
				<< " - " << e.what() << std::endl;
			// We could return early or ignore the pattern. Here, let's just return.
			return;
		}
	}

	// Monitor thread: prints status in interval
	std::thread monitor([&]() {
		using namespace std::chrono_literals;
		while (true) {
			std::this_thread::sleep_for(500ms);
			bool done = fileQueue.isFinished() && fileQueue.isEmpty();
			{
				std::lock_guard<std::mutex> lock(g_statusMutex);
				printStatusTable();
			}
			if (done) {
				break;
			}
		}
		});

	// Producer thread enumerates the directory
	std::thread producer([&]() {
		try {
			for (auto& entry : std::filesystem::recursive_directory_iterator(
				directory,
				std::filesystem::directory_options::skip_permission_denied))
			{
				try {
					if (!entry.is_regular_file()) {
						continue;
					}
					const auto& filePath = entry.path();

					auto u8name = filePath.filename().u8string();  // yields a std::u8string
					// Convert std::u8string -> std::string (raw bytes in UTF-8)
					std::string normalName(u8name.begin(), u8name.end());

					if (wildcardRegex.has_value()) {
						if (!matchesWildcard(normalName, wildcardRegex.value())) {
							continue;
						}
					}
					fileQueue.push(filePath);
				}
				catch (std::exception& e) {
					std::lock_guard<std::mutex> lock(g_statusMutex);
					auto u8 = entry.path().u8string();  // std::u8string
					std::string pathStr(u8.begin(), u8.end());  // convert to std::string by copying raw bytes
					g_status.lastError = "Error reading an entry: " + pathStr + "/n" + e.what();
				}
			}
		}
		catch (std::exception& ex) {
			std::lock_guard<std::mutex> lock(g_statusMutex);
			g_status.lastError = "Error scanning directory: " + directory.string() + "/n" + ex.what();
		}
		fileQueue.setFinished();
		});

	// 2. Spawn consumer (worker) threads
	unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
	std::vector<std::thread> workers;
	workers.reserve(numThreads);

	for (unsigned int i = 0; i < numThreads; ++i) {
		workers.emplace_back([&]() {
			while (true) {
				std::filesystem::path filePath;
				if (!fileQueue.pop(filePath)) {
					break;
				}

				{
					std::lock_guard<std::mutex> lock(g_statusMutex);
					auto u8 = filePath.u8string();  // std::u8string
					g_status.currentFile = std::string(u8.begin(), u8.end());
					g_status.fileHits = 0; // reset each time we start a new file
				}
				searchInFile(filePath, query, use_regex);
			}
			});
	}

	// 3. Wait for producer to finish
	producer.join();

	// 4. Wait for all consumers
	for (auto& w : workers) {
		w.join();
	}

	// Wait for monitor
	monitor.join();

	// Optionally print a final summary
	{
		std::lock_guard<std::mutex> lock(g_statusMutex);
		printStatusTable();
	}
}


static std::string wildcardToRegex(const std::string& wildcard)
{
	std::string regexStr;
	regexStr.reserve(wildcard.size() * 2 + 2);
	regexStr.append("^"); // match start of string

	for (char c : wildcard) {
		switch (c) {
		case '*':
			// .* => matches zero or more of any character
			regexStr.append(".*");
			break;
		case '?':
			// . => matches exactly one of any character
			regexStr.append(".");
			break;
			// Escape regex special characters
		case '.':
		case '\\':
		case '+':
		case '^':
		case '$':
		case '(':
		case ')':
		case '{':
		case '}':
		case '[':
		case ']':
		case '|':
		case '/':
			regexStr.push_back('\\');
			regexStr.push_back(c);
			break;
		default:
			// Ordinary character
			regexStr.push_back(c);
			break;
		}
	}

	regexStr.append("$"); // match end of string
	return regexStr;
}