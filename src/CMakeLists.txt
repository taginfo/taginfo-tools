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

add_executable(taginfo-similarity taginfo-similarity.cpp)
target_include_directories(taginfo-similarity SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(taginfo-similarity PRIVATE ${WARNINGS})
target_link_libraries(taginfo-similarity PRIVATE ${SQLITE_LIBRARY})

add_executable(taginfo-sizes taginfo-sizes.cpp)
target_include_directories(taginfo-sizes SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${OSMIUM_INCLUDE_DIRS})
target_compile_options(taginfo-sizes PRIVATE ${WARNINGS})

add_executable(taginfo-stats taginfo-stats.cpp tagstats-handler.cpp)
target_include_directories(taginfo-stats SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${OSMIUM_INCLUDE_DIRS})
target_compile_options(taginfo-stats PRIVATE ${WARNINGS})
target_link_libraries(taginfo-stats PRIVATE ${OSMIUM_LIBRARIES} ${GD_LIBRARY} ${SQLITE_LIBRARY})
set_pthread_on_target(taginfo-stats)

add_executable(taginfo-unicode taginfo-unicode.cpp)
target_include_directories(taginfo-unicode SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include ${ICU_INCLUDE_DIR})
target_compile_options(taginfo-unicode PRIVATE ${WARNINGS})
target_link_libraries(taginfo-unicode PRIVATE ${SQLITE_LIBRARY} ${ICU_IO_LIBRARY} ${ICU_UC_LIBRARY})

install(TARGETS taginfo-similarity taginfo-stats taginfo-unicode DESTINATION bin)

#-----------------------------------------------------------------------------
