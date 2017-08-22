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


add_subdirectory(CommonUtilitiesLib)
add_subdirectory(HTTPUtilitiesLib)


set(HEADER_FILES )

set(SOURCE_FILES
        main.cpp)

add_executable(demo ${SOURCE_FILES})
target_link_libraries(demo
        PRIVATE CommonUtilities
        PRIVATE HTTPUtilities)