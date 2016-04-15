include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_backend STATIC
        SourcesToolBackend.cc DashboardService.cc SourcesToolService.cc)
target_link_libraries(primarysources_backend
        primarysources_model primarysources_persistence primarysources_redis)

add_library(primarysources_redis STATIC RedisCacheService.cc RedisCacheService.h)
target_link_libraries(primarysources_redis primarysources_util ${REDOX_LIBRARY} ${HIREDIS_LIBRARY} ${EV_LIBRARY})