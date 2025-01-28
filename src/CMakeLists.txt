# This subdirectory builds the main utility
set(SOURCES
    dirscan.cpp
    main.cpp
)

add_executable(dirscan ${SOURCES})

# If you plan to use a library for regex, link it here (usually not needed with std::regex).
# For standard library usage, there's no extra link needed, typically.
# target_link_libraries(my_grep_like_util <library>)
