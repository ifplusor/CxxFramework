set(HEADER_FILES
        include/CF/Net/Http/HTTPProtocol.h
        include/CF/Net/Http/HTTPPacket.h
        include/CF/Net/Http/HTTPDef.h
        include/CF/Net/Http/HTTPRequestStream.h
        include/CF/Net/Http/HTTPResponseStream.h
        include/CF/Net/Http/HTTPSessionInterface.h
        include/CF/Net/Http/HTTPSession.h
        include/CF/Net/Http/HTTPListenerSocket.h
        include/CF/Net/Http/HTTPClientRequestStream.h
        include/CF/Net/Http/HTTPClientResponseStream.h
        include/CF/Net/Http/HTTPDispatcher.h
        include/CF/Net/Http/UserAgentParser.h
        include/CF/Net/Http/QueryParamList.h)

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
        HTTPDispatcher.cpp
        UserAgentParser.cpp
        QueryParamList.cpp)

add_library(CFHttp STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFHttp
        PUBLIC include)
target_link_libraries(CFHttp
        PUBLIC CFSocket)
target_compile_options(CFHttp
        PRIVATE -fPIC)