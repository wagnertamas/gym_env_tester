cmake_minimum_required(VERSION 3.29)
project(C_GUI C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED COMPONENTS SDL2)

# Include the root directory for header files
include_directories(${CMAKE_SOURCE_DIR})

# Define the source files
set(MY_SOURCES main.c open62541.c)

add_executable(C_GUI ${MY_SOURCES})
target_link_libraries(C_GUI PRIVATE SDL2::SDL2)