# Directory Scanner

## Overview

This project implements a multi-threaded, recursive “grep-like” utility in C++ that scans a given directory for a substring (or optional regex) match.  
Key aspects:

1. **Multi-threading**: Uses a **bounded producer-consumer queue** to handle large directories without storing all file paths in memory at once.
2. **File contents**: Each file is opened and read line by line, searching for the query.
3. **Results file**: Detailed matches are saved to a file named **`search_results.txt`**.
4. **Status updates**: The console displays a “status table” that updates periodically, showing how many files have been scanned, which file is currently being processed, and total hits so far.
5. **Build system**: Uses **CMake** (minimum 3.14) for Windows (MSVC) or Linux (GCC/Clang) compatibility.
6. **Tests**: Includes a small test that verifies at least the presence of matches for a known test file.

## Features

- **BoundedQueue** lowers resource consumption use by capping the number of file paths in the queue.
- **Wildcard/Extension Filtering**: Supports `--ext "*.txt"` or similar patterns (via a wildcard-to-regex conversion) to limit which files are scanned.
- **Highlighting/Truncation**: Optionally inserts ANSI color codes around the matched substring, truncates the line to ~180 characters for readability, and sanitizes unprintable characters in the output.

## Directory Layout

A typical project structure might be:

`dirscan/
├── CMakeLists.txt
├── README.md
├── src
│   ├── dirscan.h
│   ├── dirscan.cpp
│   ├── bounded_file_queue.h / bounded_file_queue.cpp
│   └── main.cpp       (Optional: a CLI entry point)
├── tests
│   ├── CMakeLists.txt
│   └── test_dirscan.cpp
└── ...` 

## Design Details

1. **BoundedFileQueue** (Producer-Consumer):
   
   - A single producer thread enumerates files in a recursive directory iteration.
   - If `MAX_QUEUE_SIZE` is reached, the producer waits until a consumer thread pops an item.
   - Consumer threads each pop file paths, call `searchInFile(...)`, and log any matches.

2. **Recursive Search**:
   
   - Uses `std::filesystem::recursive_directory_iterator`. Skips non-regular files.
   - May filter filenames if `--ext "*.txt"` or other wildcard is provided (converted to a `std::regex`).

3. **File Content Search**:
   
   - Opens each file in binary mode, reads line by line.
   - If `--regex` is specified, we compile a `std::regex` and use `regex_search()`. Otherwise, a simple `std::string::find(...)` is used for substring matching.

4. **Results Output**:
   
   - All matches are appended to `search_results.txt`. For each file that has hits, we write:
     
     `Matches in file: /path/to/file (N hits)
        Line X: [Truncated + highlighted line]
        ...` 
   
   - A separate console “status table” is updated every half-second (or 2 seconds) showing the progress and any errors.

5. **Thread Safety**:
   
   - Uses separate mutexes for printing status, writing the results file, and storing global status (e.g., “currentFile”, “filesScanned”, etc.).

## Building

**Prerequisites**:

- **CMake** 3.14 or higher.
- A C++17+ compiler (MSVC, GCC, or Clang).

### Windows (Visual Studio or Command Prompt)

1. **Open** a terminal like “x64 Native Tools Command Prompt for VS 2019” (or newer).

2. **Navigate** to your project directory.

3. **Create** a build folder:
   
   `mkdir build
   cd build` 

4. **Configure** with CMake:
   
   `cmake .. -G "Visual Studio 17 2022" -A x64` 
   
   *(Pick the generator matching your installed VS version.)*

5. **Build**:
   
   `cmake --build . --config Release` 
   
   This produces an executable`dirscan.exe` under `build/src/Release` by default.

### Linux (GCC or Clang)

1. **Navigate** to your project directory.

2. **Create** a build folder:
   
   `mkdir build && cd build` 

3. **Configure**:
   
   `cmake ..` 
   
   Optionally specify `-DCMAKE_BUILD_TYPE=Release` for an optimized build.

4. **Build**:
   
   `cmake --build .` 
   
   The resulting executable typically ends up in `build/src/dirscan`.

## Usage

Once built, run the executable from the build directory. For instance:

`dirscan "<query>" <directory> [--regex] [--ext *.txt]` 

**Example**:

`./dirscan"needle" /home/user/docs` 

or

`dirscan"^[A-Z]" C:\Data --regex --ext "*.log"` 

While it runs, it will:

1. Create (or overwrite) `search_results.txt`.
2. Print a periodically updated status table to the console, showing how many files are scanned so far, the current file, total hits, etc.
3. Once complete, check `search_results.txt` for the actual match lines.

## Testing

We provide a simple **test** that creates a temporary directory, places a couple of files (`file1.txt` containing a known match, and `file2.txt` without), then runs the utility. It verifies the expected matches are recorded in `search_results.txt`.

### Running the Tests

1. **Build** the test target (often done by default when you build the entire project).

2. **Run** the test binary directly or use `ctest`. For example, on Linux:
   
   `cd build
   ctest --verbose` 
   
   or
   
   `./tests/test_dirscan` 
   
   On Windows (Visual Studio), you can open the “Test Explorer” or run from the command line:
   
   `ctest -C Debug --verbose` 
   
   (Adjust `-C Debug` or `Release` to match your build config.)

The test will:

1. Create a **temporary** folder under the system’s temp path.
2. Write `file1.txt` (with “needle”) and `file2.txt` (without).
3. Invoke `searchInDirectory("needle", ...)`.
4. Inspect `search_results.txt` to ensure that only `file1.txt` is listed.
5. Clean up after itself.

## Known Limitations

- No PDF or other binary parsing: only raw ASCII/UTF-8 text.
- Large directories are handled well, but if you try to highlight or log every single file name to the console, that may slow things down.
- ANSI color codes in `search_results.txt` will not appear colored if you open it in standard Notepad++ or similar editors. They only show color in terminals that support ANSI.



---

**Enjoy!**
