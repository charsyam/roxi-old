cmake_minimum_required(VERSION 2.6)
project(roxi)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
)

FIND_LIBRARY( JSON_LIBRARY json PATHS 
            libs )
FILE(GLOB SRCS src/*.cpp)
FILE(GLOB HDRS include/*.h)

ADD_EXECUTABLE(roxi
    ${SRCS}
    ${HDRS}
)

TARGET_LINK_LIBRARIES( roxi pthread ${JSON_LIBRARY} )
