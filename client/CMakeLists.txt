cmake_minimum_required(VERSION 3.30)
project(client C)

set(CMAKE_C_STANDARD 11)

# Find the json-c library (ensure it's installed)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_C REQUIRED json-c)

# Add the executable
add_executable(client main.c)

# Link the json-c library to your project
target_link_libraries(client ${JSON_C_LIBRARIES})

# Include the json-c headers
target_include_directories(client PRIVATE ${JSON_C_INCLUDE_DIRS})