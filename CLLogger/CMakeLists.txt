cmake_minimum_required(VERSION 3.10)

project(CLLogger)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES "*.c*")

add_executable(CLLogger ${SOURCES})

install(TARGETS CLLogger DESTINATION bin)