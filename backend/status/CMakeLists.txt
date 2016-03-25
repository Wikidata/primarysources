include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
configure_file("Version.cc.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.cc" @ONLY)

include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/../model)
add_library(primarysources_status STATIC SystemStatus.cc "${CMAKE_CURRENT_BINARY_DIR}/Version.cc")