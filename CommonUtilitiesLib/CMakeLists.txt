set(HEADER_FILES
        include/Format.h
        include/atomic.h
        include/getopt.h
        )

set(SOURCE_FILES
        Format.cpp
        atomic.cpp
        getopt.c
        )

add_library(CommonUtilities STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CommonUtilities
        PUBLIC include
        PUBLIC ${PROJECT_SOURCE_DIR}/ThirdParty)
target_compile_options(CommonUtilities
        PRIVATE -fPIC)
