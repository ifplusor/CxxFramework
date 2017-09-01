cmake_minimum_required(VERSION 3.5)
project(CxxFramework)

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(DetectTarget)

# set language standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# detect Core and architecture
target_platform(CONF_PLATFORM)
target_architecture(CONF_ARCH)

# configure options
message(STATUS "Build for ${CONF_PLATFORM}-${CONF_ARCH}")
if (${CONF_PLATFORM} STREQUAL "OSX")
    set(CONF_DLIB_EXT so)
    add_definitions(-D__MacOSX__)
elseif (${CONF_PLATFORM} STREQUAL "Linux")
    set(CONF_DLIB_EXT so)
    add_definitions(-D__linux__)
elseif (${CONF_PLATFORM} STREQUAL "Win32")
    set(CONF_DLIB_EXT dll)
    add_definitions(-D__Win32__)
elseif (${CONF_PLATFORM} STREQUAL "MinGW")
    set(CONF_DLIB_EXT dll)
    add_definitions(-D__MinGW__)
else ()
    message(FATAL_ERROR "no configure for ${CMAKE_SYSTEM_NAME}-${CONF_PLATFORM}, quit!")
endif ()

# find dependencies
find_package(Boost)
if (NOT Boost_FOUND)
    message(WARNING "can not found boost!")
endif ()

#add_subdirectory(ThirdParty)

include_directories(Include)

add_subdirectory(CFBasic)
add_subdirectory(CFCore)
add_subdirectory(CFThread)
add_subdirectory(CFSocket)
add_subdirectory(CFHttp)

set(HEADER_FILES
        Include/CF/Platform.h
        Include/CF/Types.h
        Include/CF/Revision.h
        Include/CF/CFDef.h
        Include/CF/CFEnv.h
        Include/CF/CFConfigure.h
        Include/CF/CF.h
        Include/CF/CFState.h)

set(SOURCE_FILES
        CFEnv.cpp
        CFMain.cpp)

if (${CONF_PLATFORM} STREQUAL "Win32")
    add_library(CxxFramework STATIC
            ${HEADER_FILES} ${SOURCE_FILES})
elseif (${CONF_PLATFORM} STREQUAL "MinGW")
    add_library(CxxFramework STATIC
            ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(CxxFramework SHARED
            ${HEADER_FILES} ${SOURCE_FILES})
endif ()
target_include_directories(CxxFramework
        PUBLIC Include)
target_link_libraries(CxxFramework
        PUBLIC CFBasic
        PUBLIC CFCore
        PUBLIC CFThread
        PUBLIC CFSocket
        PUBLIC CFHttp)
target_compile_options(CxxFramework
        PRIVATE -fPIC)

add_executable(demo
        demo.cpp)
target_link_libraries(demo
        PRIVATE CxxFramework)
