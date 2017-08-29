set(HEADER_FILES
        include/ev.h
        include/ClientSocket.h
        include/EventContext.h
        include/Socket.h
        include/SocketUtils.h
        include/TCPListenerSocket.h
        include/TCPSocket.h
        include/UDPDemuxer.h
        include/UDPSocket.h
        include/UDPSocketPool.h)

set(SOURCE_FILES
        ClientSocket.cpp
        EventContext.cpp
        Socket.cpp
        SocketUtils.cpp
        TCPListenerSocket.cpp
        TCPSocket.cpp
        UDPDemuxer.cpp
        UDPSocket.cpp
        UDPSocketPool.cpp)

if (${CONF_PLATFORM} STREQUAL "Linux")
    set(SOURCE_FILES ${SOURCE_FILES} epollev.cpp)
elseif (${CONF_PLATFORM} STREQUAL "Win32")
    set(SOURCE_FILES ${SOURCE_FILES} win32ev.cpp)
elseif (${CONF_PLATFORM} STREQUAL "MinGW")
    set(SOURCE_FILES ${SOURCE_FILES} win32ev.cpp)
else (${CONF_PLATFORM} STREQUAL "OSX")
    set(HEADER_FILES ${HEADER_FILES} include/tempcalls.h)
#    set(SOURCE_FILES ${SOURCE_FILES} ev.cpp)
endif ()

add_library(CFSocket STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFSocket
        PUBLIC include)
target_link_libraries(CFSocket
        PUBLIC CFThread
        PUBLIC CFBasic)
target_compile_options(CFSocket
        PRIVATE -fPIC)

if (${CONF_PLATFORM} STREQUAL "Win32")
    target_link_libraries(CFSocket
            PUBLIC WS2_32)
elseif (${CONF_PLATFORM} STREQUAL "MinGW")
    target_link_libraries(CFSocket
            PUBLIC WS2_32)
    target_compile_options(CFSocket
            PRIVATE -fpermissive)
endif ()
