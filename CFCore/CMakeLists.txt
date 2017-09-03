set(HEADER_FILES
        include/CF/sstdlib.h
        include/CF/MyAssert.h
        include/CF/StrPtrLen.h
        include/CF/StringParser.h
        include/CF/StringFormatter.h
        include/CF/ResizeableStringFormatter.h
        include/CF/StringTranslator.h
        include/CF/Core/DateTranslator.h
        include/CF/Core/Mutex.h
        include/CF/Core/Cond.h
        include/CF/Core/Time.h
        include/CF/Core/Thread.h
        include/CF/Core/RWMutex.h
        include/CF/Queue.h
        include/CF/Heap.h
        include/CF/HashTable.h
        include/CF/Ref.h
        include/CF/Core/Utils.h
        include/CF/ConcurrentQueue.h
        include/CF/PLDoubleLinkedList.h
        include/CF/Core.h)

set(SOURCE_FILES
        InternalStdLib.cpp
        MyAssert.cpp
        StrPtrLen.cpp
        StringParser.cpp
        StringFormatter.cpp
        ResizeableStringFormatter.cpp
        StringTranslator.cpp
        DateTranslator.cpp
        Mutex.cpp
        Cond.cpp
        Time.cpp
        Thread.cpp
        RWMutex.cpp
        Queue.cpp
        Heap.cpp
        Ref.cpp
        Utils.cpp
        ConcurrentQueue.cpp)

add_library(CFCore STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFCore
        PUBLIC include)

if (NOT (${CONF_PLATFORM} STREQUAL "Win32"))
    target_compile_options(CFCore
            PRIVATE -fPIC)
endif ()

if (__PTHREADS__)
    target_link_libraries(CFCore
            PUBLIC pthread)
endif ()

if ((${CONF_PLATFORM} STREQUAL "Win32") OR (${CONF_PLATFORM} STREQUAL "MinGW"))
    target_link_libraries(CFCore
            PUBLIC winmm)
endif ()
