cmake_minimum_required(VERSION 3.5)
project(zmq-demo C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE release CACHE STRING "Choose the type of build" FORCE)
else(NOT CMAKE_BUILD_TYPE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Build type: Debug")
        add_definitions(-DDBG_TIME)
        set(CMAKE_CXX_FLAGS "-g -O0 ")
    endif()
endif()

# cmake tools 
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)

set(ZMQ_LIB ${PROJECT_SOURCE_DIR}/libs/lib/libzmq.a) 
list(APPEND 
    LINK_LIBS 
    ${ZMQ_LIB}
    pthread)

# include directory
include_directories(
    ${PROJECT_SOURCE_DIR}/libzmq/include
    ${PROJECT_SOURCE_DIR}/libs/include
)

add_subdirectory(cpp)

# print
message("===============================================================================")
string_status("Build type"          ${CMAKE_BUILD_TYPE})
string_status("C compiler"          ${CMAKE_C_COMPILER})
string_status("C++ compiler"        ${CMAKE_CXX_COMPILER})
message("===============================================================================")

