include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_backend STATIC
        SourcesToolBackend.cc DashboardService.cc SourcesToolService.cc RedisCacheService.cc RedisCacheService.h)
target_link_libraries(primarysources_backend
        primarysources_model primarysources_persistence
        ${REDOX_LIBRARY})