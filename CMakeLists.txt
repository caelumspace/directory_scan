cmake_minimum_required(VERSION 3.14)
project(DirScan VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to 20, fallback to 17 or 14 if desired
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable all warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Include subdirectories
add_subdirectory(src)
add_subdirectory(tests)
