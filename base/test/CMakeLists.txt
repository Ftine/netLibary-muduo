#Timestamp_test
add_executable(timestamp_test Timestamp_test.cpp)
target_link_libraries(timestamp_test muduo_base)
add_test(NAME timestamp_test COMMAND timestamp_test)

#Date_test
add_executable(date_test Date_test.cpp)
target_link_libraries(date_test muduo_base)
add_test(NAME date_test COMMAND date_test)

#TimeZone_test
add_executable(timezone_test TimeZone_test.cpp)
target_link_libraries(timezone_test muduo_base)
add_test(NAME timezone_test COMMAND timezone_test)

#Thread_test
add_executable(thread_test Thread_test.cpp)
target_link_libraries(thread_test muduo_base)
add_test(NAME thread_test COMMAND thread_test)

#Thread_test
add_executable(threadPool_test ThreadPool_test.cpp)
target_link_libraries(threadPool_test muduo_base)
add_test(NAME threadPool_test COMMAND threadPool_test)

#BlockingQueue_test
add_executable(blockingqueue_test BlockingQueue_test.cpp)
target_link_libraries(blockingqueue_test muduo_base)
add_test(NAME blockingqueue_test COMMAND blockingqueue_test)

#LogStream_test
add_executable(logstream_test LogStream_test.cpp)
target_link_libraries(logstream_test muduo_base)
add_test(NAME logstream_test COMMAND logstream_test)

#Logging_test
add_executable(logging_test Logging_test.cpp)
target_link_libraries(logging_test muduo_base)
add_test(NAME logging_test COMMAND logging_test)

#LogFile_test
add_executable(logfile_test LogFile_test.cpp)
target_link_libraries(logfile_test muduo_base)
add_test(NAME logfile_test COMMAND logfile_test)

#AsyncLogging_test
add_executable(asyncLogging_test AsyncLogging_test.cpp)
target_link_libraries(asyncLogging_test muduo_base)
add_test(NAME asyncLogging_test COMMAND asyncLogging_test)