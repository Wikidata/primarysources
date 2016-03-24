include_directories(..)

add_library(gtest STATIC gtest.h gtest-all.cc)

add_executable(persistence_tests PersistenceTest.cc main.cc)
target_link_libraries(persistence_tests gtest primarysources_persistence ${CPPDB_SQLITE_BACKEND})

add_executable(model_tests StatementTest.cc main.cc)
target_link_libraries(model_tests gtest primarysources_persistence ${CPPDB_SQLITE_BACKEND})

add_executable(parser_tests ParserTest.cc main.cc)
target_link_libraries(parser_tests gtest primarysources_persistence ${CPPDB_SQLITE_BACKEND})

add_test(NAME ModelTest
         COMMAND model_tests)

add_test(NAME ParserTest
         COMMAND parser_tests)

add_test(NAME PersistenceTest
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         COMMAND persistence_tests)

