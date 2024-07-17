execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

if ("${GIT_REV}" STREQUAL "")
    set(GIT_VERSION "N/A")
else()
    execute_process(COMMAND git describe --dirty --always OUTPUT_VARIABLE GIT_VERSION ERROR_QUIET)
    string(STRIP "${GIT_VERSION}" GIT_VERSION)
endif()

set(VERSION "const char* GIT_VERSION=\"${GIT_VERSION}\";")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp "${VERSION}")
endif()
