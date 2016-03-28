include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_parser STATIC Parser.cc)
target_link_libraries(primarysources_parser primarysources_model ${RE2_LIBRARY})
