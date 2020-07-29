#-----------------------------------------------------------------------------
#
#  CMakeLists.txt
#
#  Taginfo Tools
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

project(taginfo-tools VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)

#-----------------------------------------------------------------------------

find_package(ICU REQUIRED COMPONENTS io uc)
find_package(Osmium 2.14.2 REQUIRED COMPONENTS io)

find_library(GD_LIBRARY NAMES gd)
find_library(SQLITE_LIBRARY NAMES sqlite3)


#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy-10 clang-tidy-9 clang-tidy-8)

if(CLANG_TIDY)
    message(STATUS "Looking for clang-tidy - found ${CLANG_TIDY}")

    file(GLOB _srcs RELATIVE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/*.cpp")

    add_custom_target(clang-tidy
        ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        ${_srcs}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src"
    )
else()
    message(STATUS "Looking for clang-tidy - not found")
    message(STATUS "  Build target 'clang-tidy' will not be available.")
endif()


#-----------------------------------------------------------------------------

option(ERROR_ON_WARNINGS "Generate error for compiler warnings" OFF)
set(WARNING_OPTIONS -Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast CACHE STRING "Warning options")

add_subdirectory(src)

enable_testing()
add_subdirectory(test)


#-----------------------------------------------------------------------------
