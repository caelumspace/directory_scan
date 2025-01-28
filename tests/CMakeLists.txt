# In src/CMakeLists.txt, define a library for dirscan
add_library(dirscan_lib ../src/dirscan.cpp)
target_include_directories(dirscan_lib PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>)

# Then link it into the main executable
#add_executable(dirscan main.cpp)
#target_link_libraries(dirscan PRIVATE dirscan_lib)

# In tests/CMakeLists.txt
add_executable(dirscan_tests test_dirscan.cpp)
target_link_libraries(dirscan_tests PRIVATE dirscan_lib)


add_test(NAME MydirscanTests COMMAND dirscan_tests)
