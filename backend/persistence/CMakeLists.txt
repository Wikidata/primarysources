include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_persistence STATIC Persistence.cc)
target_link_libraries(primarysources_persistence
        primarysources_model primarysources_parser
        ${CMAKE_THREAD_LIBS_INIT} ${LIBCPPCMS_LIBRARIES} ${LIBCPPDB_LIBRARY} ${Boost_LIBRARIES})

