set(HEADER_FILES
        include/HTTPProtocol.h
        include/HTTPPacket.h
        include/HTTPDef.h
        include/HTTPRequestStream.h
        include/HTTPResponseStream.h
        include/HTTPSessionInterface.h
        include/HTTPSession.h
        include/HTTPListenerSocket.h
        include/HTTPClientRequestStream.h
        include/HTTPClientResponseStream.h
        include/HTTPDispatcher.h)

set(SOURCE_FILES
        HTTPClientRequestStream.cpp
        HTTPClientResponseStream.cpp
        HTTPProtocol.cpp
        HTTPPacket.cpp
        HTTPRequestStream.cpp
        HTTPResponseStream.cpp
        HTTPSessionInterface.cpp
        HTTPSession.cpp
        HTTPListenerSocket.cpp
        HTTPDispatcher.cpp)

add_library(CFHttp STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFHttp
        PUBLIC include)
target_link_libraries(CFHttp
        PUBLIC CFSocket)
target_compile_options(CFHttp
        PRIVATE -fPIC)