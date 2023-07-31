#set(ALL_FILES_H_CPP
#        Timestamp.cpp
#        copyable.h
#        Types.h
#        noncopyable.h
#        Atomic.h
#        Mutex.h
#        BlockingQueue.h
#        StringPiece.h
#        Date.cpp Date.h
#        CurrentThread.cpp CurrentThread.h
#        Thread.cpp Thread.h
#        TimeZone.cpp TimeZone.h
#        Condition.cpp Condition.h
#        CountDownlatch.cpp CountDownlatch.h
#        Exception.cpp Exception.h
#        ThreadPool.cpp ThreadPool.h
#        LogStream.cpp LogStream.h
#        BoundedBlockingQueue.h
#        Logging.h Logging.cpp
#        LogFile.cpp LogFile.h
#        FileUtil.cpp FileUtil.h
#        ProcessInfo.cpp ProcessInfo.h
#        AsyncLogging.cpp AsyncLogging.h)
#
#add_library(muduo_base ${ALL_FILES_H_CPP})
#target_link_libraries(muduo_base pthread rt)
#
#ADD_SUBDIRECTORY(test)

set(base_SRCS
        AsyncLogging.cpp
        Condition.cpp
        CountDownlatch.cpp
        CurrentThread.cpp
        Date.cpp
        Exception.cpp
        FileUtil.cpp
        LogFile.cpp
        Logging.cpp
        LogStream.cpp
        ProcessInfo.cpp
        Timestamp.cpp
        Thread.cpp
        ThreadPool.cpp
        TimeZone.cpp)

add_library(muduo_base ${base_SRCS})
target_link_libraries(muduo_base pthread rt)

install(TARGETS muduo_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/muduo/base)


add_subdirectory(test)

