include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_serializer STATIC SerializerJSON.cc SerializerTSV.cc)
target_link_libraries(primarysources_serializer primarysources_model)
