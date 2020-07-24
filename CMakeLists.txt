#-----------------------------------------------------------------------------
#
#  CMakeLists.txt
#
#  Taginfo Tools
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.1)

project(taginfo-tools VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------

add_definitions(-Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast)

#-----------------------------------------------------------------------------

find_package(ICU REQUIRED COMPONENTS io uc)
find_package(Osmium 2.14.2 REQUIRED COMPONENTS io)

find_library(GD_LIBRARY NAMES gd)
find_library(SQLITE_LIBRARY NAMES sqlite3)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS} ${ICU_INCLUDE_DIR})


#-----------------------------------------------------------------------------

add_subdirectory(src)


#-----------------------------------------------------------------------------
