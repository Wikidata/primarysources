cmake_minimum_required(VERSION 2.8.4)
project(freebase_backend)
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(DATABASE sqlite3)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")
include_directories("${CMAKE_INSTALL_PREFIX}/include" "${CMAKE_SOURCE_DIR}")

ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    -g
    # Other flags
    -DBOOST_LOG_DYN_LINK
)

find_package (Threads)
find_package (CPPCMS REQUIRED)
find_package (CPPDB REQUIRED)
find_package (Boost 1.53.0 REQUIRED COMPONENTS iostreams regex)
find_package (Tcmalloc)

if(DATABASE STREQUAL sqlite3 AND CPPDB_SQLITE_BACKEND)
    find_library(SQLITE3_LIB sqlite3)
    set(DB_LIBS ${CPPDB_SQLITE_BACKEND} ${SQLITE3_LIB})
    add_definitions(-DDB_SQLITE3)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.cc.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.cc" @ONLY)

add_library(persistence STATIC Parser.cc Statement.cc Persistence.cc)
target_link_libraries(persistence ${CMAKE_THREAD_LIBS_INIT} ${LIBCPPCMS_LIBRARIES} ${LIBCPPDB_LIBRARY} ${Boost_LIBRARIES})

add_library(util STATIC TimeLogger.cc MemStat.cc)

add_executable(inject inject.cc)
add_executable(deduplicate deduplicate.cc)
add_executable(freebase_backend main.cc SourcesToolService.cc SourcesToolBackend.cc SerializerTSV.cc SerializerJSON.cc "${CMAKE_CURRENT_BINARY_DIR}/Version.cc" Status.cc Dashboard.h Dashboard.cc)

target_link_libraries(freebase_backend util persistence ${DB_LIBS} ${Tcmalloc_LIBRARIES})
target_link_libraries(inject  persistence ${DB_LIBS})
target_link_libraries(deduplicate  persistence ${DB_LIBS})

install(TARGETS inject deduplicate freebase_backend
        RUNTIME DESTINATION bin)

add_subdirectory(test)
