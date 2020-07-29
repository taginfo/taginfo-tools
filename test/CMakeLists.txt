#-----------------------------------------------------------------------------
#
#  test/CMakeLists.txt
#
#  Taginfo Tools
#
#-----------------------------------------------------------------------------

# Unit tests

add_executable(unit-tests unit-tests.cpp test-hash.cpp)
target_include_directories(unit-tests SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src catch)
add_test(NAME unit-tests COMMAND unit-tests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")


# Integration tests

file(GLOB TEST_SCRIPTS t/*.sh)

list(SORT TEST_SCRIPTS)

foreach(file ${TEST_SCRIPTS})
    string(REGEX REPLACE "^.*/(.+).sh$" "\\1" tid ${file})

    message(STATUS "Adding test: ${tid}")
    add_test(NAME test-${tid}
             COMMAND ${file} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${tid})
endforeach()

