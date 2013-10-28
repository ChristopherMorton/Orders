cmake_minimum_required(VERSION 2.6)
project(Summoner)

set(EXECUTABLE_NAME Summoner)

# Enable debug symbols by default
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()
# (you can also set it on the command line: -D CMAKE_BUILD_TYPE=Release)

# Set version information in a config.h file
set(Summoner_VERSION_MAJOR 1)
set(Summoner_VERSION_MINOR 0)
include_directories("${PROJECT_BINARY_DIR}" "${PROJECT_BINARY_DIR}/include" "${PROJECT_BINARY_DIR}/lib")


# Detect and add SFML
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
#Find any version 2.X of SFML
#See the FindSFML.cmake file for additional details and instructions
find_package(SFML 2 REQUIRED system window graphics network audio)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/build)
set(CMAKE_CXX_FLAGS "-g -Wall")
add_subdirectory(src)
