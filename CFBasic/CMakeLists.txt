set(HEADER_FILES
        include/base64.h
        include/md5.h
        include/md5digest.h)

set(SOURCE_FILES
        base64.c
        md5.c
        md5digest.cpp)

add_library(CFBasic STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFBasic
        PUBLIC include)
target_link_libraries(CFBasic
        )
target_compile_options(CFBasic
        PRIVATE -fPIC)
