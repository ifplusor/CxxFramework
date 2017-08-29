set(HEADER_FILES
        include/base64.h
        include/md5.h
        include/md5digest.h
        include/GetWord.h
        include/Trim.h
        include/ConfParser.h)

set(SOURCE_FILES
        base64.c
        md5.c
        md5digest.cpp
        GetWord.c
        Trim.c
        ConfParser.cpp)

add_library(CFBasic STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFBasic
        PUBLIC include)
target_link_libraries(CFBasic
        PUBLIC CFCore)
target_compile_options(CFBasic
        PRIVATE -fPIC)
