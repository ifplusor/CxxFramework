set(HEADER_FILES
        include/ConfParser.h
        include/FastCopyMacros.h
        include/Format.h
        include/GetWord.h
        include/QueryParamList.h
        include/SDPUtils.h
        include/Trim.h
        include/UserAgentParser.h
        include/atomic.h
        include/getopt.h
        include/keyframecache.h
        include/sdpCache.h)

set(SOURCE_FILES
        ConfParser.cpp
        Format.cpp
        GetWord.c
        QueryParamList.cpp
        SDPUtils.cpp
        Trim.c
        UserAgentParser.cpp
        atomic.cpp
        getopt.c
        keyframecache.cpp
        sdpCache.cpp)

add_library(CommonUtilities STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CommonUtilities
        PUBLIC include
        PUBLIC ${PROJECT_SOURCE_DIR}/ThirdParty)
target_compile_options(CommonUtilities
        PRIVATE -fPIC)
