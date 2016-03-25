include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_backend STATIC SourcesToolBackend.cc SourcesToolService.cc)