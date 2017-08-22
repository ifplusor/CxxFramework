set(HEADER_FILES
        ${PROJECT_SOURCE_DIR}/Include/QTSS.h
        include/HTTPRequestStream.h
        include/HTTPResponseStream.h
        include/HTTPSessionInterface.h
        include/HTTPSession.h
        include/HTTPListenerSocket.h)

set(SOURCE_FILES
        HTTPRequestStream.cpp
        HTTPResponseStream.cpp
        HTTPSessionInterface.cpp
        HTTPSession.cpp
        HTTPListenerSocket.cpp)

add_library(HTTPUtilities STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(HTTPUtilities
        PUBLIC include
        PUBLIC ${PROJECT_SOURCE_DIR}/Include)
target_link_libraries(HTTPUtilities
        PUBLIC CommonUtilities)