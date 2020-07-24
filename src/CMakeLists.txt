#-----------------------------------------------------------------------------
#
#  src/CMakeLists.txt
#
#  Taginfo Tools
#
#-----------------------------------------------------------------------------

set(WARNINGS -Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast)

add_executable(osmstats osmstats.cpp)
target_include_directories(osmstats SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${OSMIUM_INCLUDE_DIRS})
target_compile_options(osmstats PRIVATE ${WARNINGS})
target_link_libraries(osmstats PRIVATE ${OSMIUM_LIBRARIES} ${SQLITE_LIBRARY})
set_pthread_on_target(osmstats)

add_executable(similarity similarity.cpp)
target_include_directories(similarity SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(similarity PRIVATE ${WARNINGS})
target_link_libraries(similarity PRIVATE ${SQLITE_LIBRARY})

add_executable(sizes sizes.cpp)
target_include_directories(sizes SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${OSMIUM_INCLUDE_DIRS})
target_compile_options(sizes PRIVATE ${WARNINGS})

add_executable(taginfo_unicode taginfo_unicode.cpp)
target_include_directories(taginfo_unicode SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${ICU_INCLUDE_DIR})
target_compile_options(taginfo_unicode PRIVATE ${WARNINGS})
target_link_libraries(taginfo_unicode PRIVATE ${SQLITE_LIBRARY} ${ICU_IO_LIBRARY} ${ICU_UC_LIBRARY})

add_executable(tagstats tagstats.cpp tagstats_handler.cpp)
target_include_directories(tagstats SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${OSMIUM_INCLUDE_DIRS})
target_compile_options(tagstats PRIVATE ${WARNINGS})
target_link_libraries(tagstats PRIVATE ${OSMIUM_LIBRARIES} ${GD_LIBRARY} ${SQLITE_LIBRARY})
set_pthread_on_target(tagstats)

install(TARGETS similarity taginfo_unicode tagstats DESTINATION bin)

#-----------------------------------------------------------------------------
