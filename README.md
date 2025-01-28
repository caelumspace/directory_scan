# My Grep-Like Utility

## Overview
This project implements a simplified `grep`-like utility in C++ that recursively searches for a given query string (or regex) within the contents of all files in a directory. Key requirements and considerations:

- **Only C++ standard library** is used (up to C++20).
- **Multi-threaded** approach to utilize all CPU cores, speeding up the search.
- Works on Linux (GCC), macOS (Clang), and Windows (MSVC).
- **CMake** build system.

## Design
1. **Directory Traversal**  
   Uses `std::filesystem::recursive_directory_iterator` to list files.  
   Skips non-regular files and respects any extension filter if provided (e.g., `--ext .txt`).

2. **Producer-Consumer Model**  
   - A single producer thread traverses directories and pushes file paths into a thread-safe queue.  
   - Multiple consumer threads pop paths from the queue and perform file reading and string (or regex) matching.

3. **Thread Safety**  
   - A custom `FileQueue` class with a mutex and condition variable ensures synchronized access to the queue.  
   - An additional mutex for output ensures matched lines do not get interleaved on `std::cout`.

4. **Search Mechanism**  
   - Substring matching via `std::string::find(query)` by default.  
   - Optional regex matching via `std::regex` when `--regex` is specified.  
   - Output includes filename, line number, and the matching line text.

5. **Performance Considerations**  
   - Number of consumer threads = `std::thread::hardware_concurrency()`.  
   - Streams line-by-line to avoid loading entire files into memory at once.

## Build Instructions
1. **Prerequisites**  
   - A C++ compiler (GCC, Clang, or MSVC) supporting at least C++17 or C++20.  
   - CMake v3.14 or higher.  
   
	mkdir build && cd build
	cmake ..
	cmake --build .
	
	***Running Tests***
	./tests/grep_tests

	

2. **Cloning the Repository**  
   ```bash
   git clone <repo-url> my-grep-like-utility
   cd my-grep-like-utility
