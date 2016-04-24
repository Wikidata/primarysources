include_directories(. ..)

add_library(gtest STATIC gtest/gtest.h gmock/gmock.h gmock-gtest-all.cc main.cc)

add_executable(persistence_tests PersistenceTest.cc)
target_link_libraries(persistence_tests gtest primarysources_persistence ${CPPDB_SQLITE_BACKEND} ${GLOG_LIBRARY})

add_executable(model_tests StatementTest.cc)
target_link_libraries(model_tests gtest primarysources_persistence ${CPPDB_SQLITE_BACKEND} ${GLOG_LIBRARY})

add_executable(parser_tests ParserTest.cc)
target_link_libraries(parser_tests gtest
        primarysources_model primarysources_parser ${CPPDB_SQLITE_BACKEND} ${GLOG_LIBRARY})

add_executable(serializer_tests SerializerTest.cc)
target_link_libraries(serializer_tests gtest
        primarysources_model primarysources_serializer ${LIBCPPCMS_LIBRARIES} ${GLOG_LIBRARY})

add_executable(cache_tests CacheTest.cc)
target_link_libraries(cache_tests gtest primarysources_model ${CPPDB_SQLITE_BACKEND} ${GLOG_LIBRARY})

add_executable(util_tests SnowflakeTest.cc RetryTest.cc)
target_link_libraries(util_tests gtest primarysources_util ${CMAKE_THREAD_LIBS_INIT} ${GLOG_LIBRARY})

add_executable(redis_tests RedisTest.cc)
target_link_libraries(redis_tests gtest primarysources_model primarysources_redis
        ${CMAKE_THREAD_LIBS_INIT} ${REDOX_LIBRARY} ${HIREDIS_LIBRARY} ${EV_LIBRARY} ${GLOG_LIBRARY})

add_test(NAME ModelTest
         COMMAND model_tests)

add_test(NAME ParserTest
         COMMAND parser_tests)

add_test(NAME SerializerTest
        COMMAND serializer_tests)

add_test(NAME CacheTest
        COMMAND cache_tests)

add_test(NAME PersistenceTest
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         COMMAND persistence_tests)

add_test(NAME UtilTest
        COMMAND util_tests)

