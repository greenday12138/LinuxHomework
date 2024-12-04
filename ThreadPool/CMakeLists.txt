cmake_minimum_required(VERSION 3.10)

project(ThreadPool)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# file(GLOB SOURCES "*.c*")

add_library(main Util.cpp)

add_executable(ThreadPool main.cpp)
target_link_libraries(ThreadPool main)

add_executable(Test test.cpp)
target_link_libraries(Test main)

install(TARGETS ThreadPool Test DESTINATION bin)