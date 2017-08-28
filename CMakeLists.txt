cmake_minimum_required(VERSION 3.5)
project(CxxFramework)

# detect os
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CND_PLATFORM OSX)
    set(CND_DLIB_EXT so)
    add_definitions(-D__MacOSX__)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CND_PLATFORM Linux)
    set(CND_DLIB_EXT so)
    add_definitions(-D__linux__)
    #    add_compile_options(-O2)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (MINGW)
        message("use mingw toolchain")
        set(CND_PLATFORM MinGW)
        add_definitions(-D__MinGW__)
    elseif (MSYS)
        message("use msys toolchain")
    else()
        message("use msvc toolchain")
        set(CND_PLATFORM Win32)
        set(CND_DLIB_EXT dll)
        add_definitions(-D__Win32__)
    endif ()
else ()
    message("no build confg for ${CMAKE_SYSTEM_NAME}")
endif ()

# detect architecture
include(TargetArch.cmake)
target_architecture(CND_ARCH)

message("build for ${CND_PLATFORM}-${CND_ARCH}")

# set language standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)


include_directories(Include)

add_subdirectory(CFBasic)
add_subdirectory(CFCore)
add_subdirectory(CFThread)
add_subdirectory(CFSocket)
add_subdirectory(CFHttp)

set(HEADER_FILES
        Include/PlatformHeader.h
        Include/OSHeaders.h
        Include/revision.h
        Include/CFDef.h
        Include/CFEnv.h
        Include/CFConfigure.h
        Include/CF.h
        Include/CFState.h)

set(SOURCE_FILES
        CFEnv.cpp
        CFMain.cpp)

if (${CND_PLATFORM} MATCHES "Win32")
add_library(CxxFramework STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
elseif (${CND_PLATFORM} MATCHES "MinGW")
add_library(CxxFramework STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
else ()
add_library(CxxFramework SHARED
        ${HEADER_FILES} ${SOURCE_FILES})
endif ()
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
