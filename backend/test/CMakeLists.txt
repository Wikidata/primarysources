include_directories(..)

add_library(gtest STATIC gtest.h gtest-all.cc)

add_executable(persistence_tests PersistenceTest.cc ../Persistence.cc ../Statement.cc)
target_link_libraries(persistence_tests gtest ${CMAKE_THREAD_LIBS_INIT} ${LIBCPPCMS_LIBRARIES} ${LIBCPPDB_LIBRARY} ${CPPDB_SQLITE_BACKEND} ${Boost_LIBRARIES})

add_test(NAME PersistenceTest
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         COMMAND persistence_tests)