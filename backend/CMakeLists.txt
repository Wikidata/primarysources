cmake_minimum_required(VERSION 2.8.4)
project(primarysources)
set(VERSION_MAJOR 2)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(DATABASE sqlite3)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")
include_directories(
        ${CMAKE_INSTALL_PREFIX}/include ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/model)

ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    -g
    # Other flags
    -DBOOST_LOG_DYN_LINK
)

#find_package (GFlags REQUIRED)
find_package (Protobuf 10 REQUIRED)
find_package (GLog REQUIRED)
find_package (GFlags REQUIRED)
find_package (Threads)
find_package (CPPCMS REQUIRED)
find_package (CPPDB REQUIRED)
find_package (RE2 REQUIRED)
find_package (Boost 1.53.0 REQUIRED COMPONENTS iostreams)
find_package (Tcmalloc)
find_package (Redox)
find_package (Hiredis)
find_package (EV)

if(DATABASE STREQUAL sqlite3 AND CPPDB_SQLITE_BACKEND)
    find_library(SQLITE3_LIB sqlite3)
    set(DB_LIBS ${CPPDB_SQLITE_BACKEND} ${SQLITE3_LIB})
    add_definitions(-DDB_SQLITE3)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(model)
add_subdirectory(parser)
add_subdirectory(persistence)
add_subdirectory(serializer)
add_subdirectory(service)
add_subdirectory(status)
add_subdirectory(util)

include_directories()

add_executable(primarysources_inject inject.cc)
add_executable(primarysources_deduplicate deduplicate.cc)
add_executable(primarysources_updatecache updatecache.cc)
add_executable(primarysources_service main.cc)

target_link_libraries(primarysources_service
        primarysources_backend primarysources_serializer primarysources_persistence
        primarysources_status primarysources_util
        ${DB_LIBS} ${Tcmalloc_LIBRARIES} ${GLOG_LIBRARY})
target_link_libraries(primarysources_inject  primarysources_persistence
        ${DB_LIBS} ${GLOG_LIBRARY})
target_link_libraries(primarysources_deduplicate primarysources_persistence
        ${DB_LIBS} ${GLOG_LIBRARY})
target_link_libraries(primarysources_updatecache
        primarysources_persistence primarysources_redis jsoncpp primarysources_util
        ${DB_LIBS} ${GLOG_LIBRARY} ${GFLAGS_LIBRARY})

install(TARGETS
        primarysources_inject primarysources_deduplicate primarysources_updatecache
        primarysources_service
        RUNTIME DESTINATION bin)

add_subdirectory(test)
