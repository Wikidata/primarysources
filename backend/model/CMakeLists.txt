file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ProtoFiles})
include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/..)

add_library(primarysources_model Statement.cc ${PROTO_SRCS} ${PROTO_HDRS} Status.cc Status.h)
target_link_libraries(primarysources_model ${CMAKE_THREAD_LIBS_INIT} ${PROTOBUF_LIBRARIES})