cmake_minimum_required(VERSION 2.6)
project(roxi)

find_package(Boost COMPONENTS thread system filesystem REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
)

FIND_LIBRARY( JSON_LIBRARY json PATHS 
            libs 
            /usr/lib64
            /usr/local/lib
            )

FILE(GLOB SRCS src/*.cpp)
FILE(GLOB HDRS include/*.h)

ADD_EXECUTABLE(roxi
    ${SRCS}
    ${HDRS}
)

TARGET_LINK_LIBRARIES( roxi pthread ${JSON_LIBRARY}
                            ${Boost_FILESYSTEM_LIBRARY}
                            ${Boost_SYSTEM_LIBRARY}
                            ${Boost_THREAD_LIBRARY}
)
