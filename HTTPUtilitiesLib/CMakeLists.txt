set(HEADER_FILES
        include/HTTPClientRequestStream.h
        include/HTTPClientResponseStream.h
        include/HTTPProtocol.h
        include/HTTPRequest.h
        include/HTTPRequestStream.h
        include/HTTPResponseStream.h
        include/HTTPSessionInterface.h
        include/HTTPSession.h
        include/HTTPListenerSocket.h)

set(SOURCE_FILES
        HTTPClientRequestStream.cpp
        HTTPClientResponseStream.cpp
        HTTPProtocol.cpp
        HTTPRequest.cpp
        HTTPRequestStream.cpp
        HTTPResponseStream.cpp
        HTTPSessionInterface.cpp
        HTTPSession.cpp
        HTTPListenerSocket.cpp)

add_library(HTTPUtilities STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(HTTPUtilities
        PUBLIC include)
target_link_libraries(HTTPUtilities
        PUBLIC CommonUtilities)