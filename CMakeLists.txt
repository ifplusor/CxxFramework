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
    set(CND_PLATFORM Win32)
    set(CND_DLIB_EXT dll)
    add_definitions(-D__Win32__)
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

add_subdirectory(CommonUtilitiesLib)
add_subdirectory(HTTPUtilitiesLib)

set(HEADER_FILES
        Include/PlatformHeader.h
        Include/OSHeaders.h
        Include/revision.h
        Include/CFDef.h
        Include/CFEnv.h
        Include/CFConfigure.h
        Include/CF.h)

set(SOURCE_FILES
        CFEnv.cpp
        CFConfigure.cpp)

add_library(CxxFramework SHARED
        ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(CxxFramework
        PUBLIC CommonUtilities
        PUBLIC HTTPUtilities
        PUBLIC pthread)
target_compile_options(CxxFramework
        PRIVATE -fPIC)

add_executable(demo
        main.cpp)
target_link_libraries(demo
        PRIVATE CxxFramework)
