set(HEADER_FILES
        include/CF/Thread/Task.h
        include/CF/Thread/IdleTask.h
        include/CF/Thread/TimeoutTask.h include/CF/Thread.h)

set(SOURCE_FILES
        Task.cpp
        IdleTask.cpp
        TimeoutTask.cpp)

add_library(CFThread STATIC
        ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(CFThread
        PUBLIC include)
target_link_libraries(CFThread
        PUBLIC CFCore)
target_compile_options(CFThread
        PRIVATE -fPIC)
