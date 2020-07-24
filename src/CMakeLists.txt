#-----------------------------------------------------------------------------
#
#  src/CMakeLists.txt
#
#  Taginfo Tools
#
#-----------------------------------------------------------------------------

add_executable(osmstats osmstats.cpp)
target_link_libraries(osmstats ${OSMIUM_LIBRARIES} ${SQLITE_LIBRARY})
set_pthread_on_target(osmstats)
target_compile_features(osmstats PRIVATE cxx_noexcept)

add_executable(similarity similarity.cpp)
target_link_libraries(similarity ${SQLITE_LIBRARY})
target_compile_features(similarity PRIVATE cxx_noexcept)

add_executable(sizes sizes.cpp)
target_compile_features(sizes PRIVATE cxx_noexcept)

add_executable(taginfo_unicode taginfo_unicode.cpp)
target_link_libraries(taginfo_unicode ${SQLITE_LIBRARY} ${ICU_IO_LIBRARY} ${ICU_UC_LIBRARY})
target_compile_features(taginfo_unicode PRIVATE cxx_noexcept)

add_executable(tagstats tagstats.cpp tagstats_handler.cpp)
target_link_libraries(tagstats ${OSMIUM_LIBRARIES} ${GD_LIBRARY} ${SQLITE_LIBRARY})
target_compile_features(tagstats PRIVATE cxx_noexcept)
set_pthread_on_target(tagstats)


#-----------------------------------------------------------------------------
