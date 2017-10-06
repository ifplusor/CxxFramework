cmake_minimum_required(VERSION 3.5)
project(CxxFramework)

# set language standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# detect Core and architecture
if ((NOT (DEFINED DetectTarget)) OR DetectTarget)
    message("CxxFramework: Detecting target...")
    LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
    include(DetectTarget)
    target_platform(CONF_PLATFORM)
    target_architecture(CONF_ARCH)
    set(DetectTarget FALSE)
endif ()

# configure options
# - follow options must be configure:
#    set(PLATFORM_DEFINE __linux__)
# - follow options can be configure:
#    set(DEBUG FALSE)
#    set(ASSERT FALSE)
#    set(USE_ENUM FALSE)
#    set(MMAP_TABLES FALSE)
#    set(USE_ATOMICLIB FALSE)
#    set(MACOSXEVENTQUEUE FALSE)
#    set(__PTHREADS__ FALSE)
#    set(__PTHREADS_MUTEXES__ FALSE)
#    set(ALLOW_NON_WORD_ALIGN_ACCESS FALSE)
#    set(USE_THREAD FALSE)
#    set(THREADING_IS_COOPERATIVE FALSE)
#    set(USE_THR_YIELD FALSE)
#    set(MACOSX_PUBLICBETA FALSE)
#    set(__WinSock__ FALSE)
#    set(USE_DEFAULT_STD_LIB FALSE)
message(STATUS "Build for ${CONF_PLATFORM}-${CONF_ARCH}")
if (${CONF_PLATFORM} STREQUAL "OSX")
    set(CONF_DLIB_EXT so)
    set(MACOSXEVENTQUEUE TRUE)
    set(__PTHREADS__ TRUE)
    set(__PTHREADS_MUTEXES__ TRUE)
    set(ALLOW_NON_WORD_ALIGN_ACCESS TRUE)
    set(USE_DEFAULT_STD_LIB TRUE)
elseif (${CONF_PLATFORM} STREQUAL "Linux")
    set(CONF_DLIB_EXT so)
    set(__PTHREADS__ TRUE)
    set(__PTHREADS_MUTEXES__ TRUE)
    set(ALLOW_NON_WORD_ALIGN_ACCESS TRUE)
    set(_REENTRANT TRUE)
elseif (${CONF_PLATFORM} STREQUAL "Win32")
    set(CONF_DLIB_EXT dll)
    set(__WinSock__ TRUE)
    set(ALLOW_NON_WORD_ALIGN_ACCESS TRUE)
    add_compile_options("/std:c++latest")
elseif (${CONF_PLATFORM} STREQUAL "MinGW")
    set(CONF_DLIB_EXT dll)
    set(__WinSock__ TRUE)
    set(__PTHREADS__ TRUE)
    set(__PTHREADS_MUTEXES__ TRUE)
    set(ALLOW_NON_WORD_ALIGN_ACCESS TRUE)
    set(_REENTRANT TRUE)
else ()
    message(FATAL_ERROR "no configure for ${CMAKE_SYSTEM_NAME}-${CONF_PLATFORM}, quit!")
endif ()
if (__PTHREADS__)
    add_definitions(-D_REENTRANT)
endif ()
set(DEBUG FALSE)
set(ASSERT TRUE)

# generate platform flag include file
configure_file(
        ${PROJECT_SOURCE_DIR}/Platform.h.in
        ${PROJECT_SOURCE_DIR}/Include/CF/Platform.h)

# find dependencies
#find_package(Boost)
#if (NOT Boost_FOUND)
#    message(WARNING "can not found boost!")
#endif ()

#add_subdirectory(ThirdParty)

# include common head file
include_directories(Include)

# add sub modules
add_subdirectory(CFBasic)
add_subdirectory(CFCore)
add_subdirectory(CFThread)
add_subdirectory(CFSocket)
add_subdirectory(CFHttp)

# define Cxx Framework
set(HEADER_FILES
        Include/CF/Platform.h
        Include/CF/Types.h
        Include/CF/Revision.h
        Include/CF/CFDef.h
        Include/CF/CFEnv.h
        Include/CF/CFConfigure.hpp
        Include/CF/CF.h
        Include/CF/CFState.h)

set(SOURCE_FILES
        CFEnv.cpp
        CFMain.cpp)

if ((DEFINED CxxFramework_USE_STATIC)
        OR (${CONF_PLATFORM} STREQUAL "Win32")
        OR (${CONF_PLATFORM} STREQUAL "MinGW"))
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

if (NOT (${CONF_PLATFORM} STREQUAL "Win32"))
    target_compile_options(CxxFramework
            PRIVATE -fPIC)
endif ()

add_executable(demo
        demo.cpp)
target_link_libraries(demo
        PRIVATE CxxFramework)
