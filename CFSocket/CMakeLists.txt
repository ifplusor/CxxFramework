set(HEADER_FILES
        include/CF/Net/ev.h
        include/CF/Net/Socket/ClientSocket.h
        include/CF/Net/Socket/EventContext.h
        include/CF/Net/Socket/Socket.h
        include/CF/Net/Socket/SocketUtils.h
        include/CF/Net/Socket/TCPListenerSocket.h
        include/CF/Net/Socket/TCPSocket.h
        include/CF/Net/Socket/UDPDemuxer.h
        include/CF/Net/Socket/UDPSocket.h
        include/CF/Net/Socket/UDPSocketPool.h)

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
else (${CONF_PLATFORM} STREQUAL "macOS")
    set(HEADER_FILES ${HEADER_FILES} include/CF/Net/tempcalls.h)
endif ()

add_library(CFSocket STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFSocket
        PUBLIC include)
target_link_libraries(CFSocket
        PUBLIC CFThread
        PUBLIC CFBasic)

if (NOT (${CONF_PLATFORM} STREQUAL "Win32"))
    target_compile_options(CFSocket
            PRIVATE -fPIC)
endif ()

if (__WinSock__)
    target_link_libraries(CFSocket
            PUBLIC WS2_32)
endif ()

if (${CONF_PLATFORM} STREQUAL "MinGW")
    target_compile_options(CFSocket
            PRIVATE -fpermissive)
endif ()
