set(HEADER_FILES
        include/SafeStdLib.h
        include/MyAssert.h
        include/StrPtrLen.h
        include/StringParser.h
        include/StringFormatter.h
        include/ResizeableStringFormatter.h
        include/StringTranslator.h
        include/DateTranslator.h
        include/OSMutex.h
        include/OSCond.h
        include/OSTime.h
        include/OSThread.h
        include/OSMutexRW.h
        include/OSQueue.h
        include/OSHeap.h
        include/OSHashTable.h
        include/OSRef.h
        include/OS.h)

set(SOURCE_FILES
        InternalStdLib.cpp
        MyAssert.cpp
        StrPtrLen.cpp
        StringParser.cpp
        StringFormatter.cpp
        ResizeableStringFormatter.cpp
        StringTranslator.cpp
        DateTranslator.cpp
        OSMutex.cpp
        OSCond.cpp
        OSTime.cpp
        OSThread.cpp
        OSMutexRW.cpp
        OSQueue.cpp
        OSHeap.cpp
        OSRef.cpp
        OS.cpp)

add_library(CFCore STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFCore
        PUBLIC include)
target_compile_options(CFCore
        PRIVATE -fPIC)

if (${CONF_PLATFORM} STREQUAL "Linux")
    target_link_libraries(CFCore
            PUBLIC pthread)
elseif (${CONF_PLATFORM} STREQUAL "Win32")
    target_link_libraries(CFCore
            PUBLIC winmm)
elseif (${CONF_PLATFORM} STREQUAL "MinGW")
    target_link_libraries(CFCore
            PUBLIC winmm)
endif ()
