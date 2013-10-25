cmake_minimum_required(VERSION 2.6)
project(Orders)

# find_package(SFML)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/build)
set(CMAKE_CXX_FLAGS "-Wall")
add_subdirectory(src)
