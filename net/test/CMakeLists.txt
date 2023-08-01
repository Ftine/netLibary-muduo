#EventLoop_test
add_executable(eventLoop_test EventLoop_test.cpp)
target_link_libraries(eventLoop_test muduo_net)
add_test(NAME eventLoop_test COMMAND eventLoop_test)

#Channel_test
add_executable(channel_test Channel_test.cpp)
target_link_libraries(channel_test muduo_net)
add_test(NAME channel_test COMMAND channel_test)


#EventLoopThread_test
add_executable(eventLoopThread_test EventLoopThread_test.cpp)
target_link_libraries(eventLoopThread_test muduo_net)
add_test(NAME eventLoopThread_test COMMAND eventLoopThread_test)

#EventLoopThreadPool_test
add_executable(eventLoopThreadPool_test EventLoopThreadPool_test.cpp)
target_link_libraries(eventLoopThreadPool_test muduo_net)
add_test(NAME eventLoopThreadPool_test COMMAND eventLoopThreadPool_test)

#Accpetor_test
add_executable(accpetor_test Accpetor_test.cpp)
target_link_libraries(accpetor_test muduo_net)
add_test(NAME accpetor_test COMMAND accpetor_test)



#EchoClient_test
add_executable(echoClient_test EchoClient_test.cpp)
target_link_libraries(echoClient_test muduo_net)
add_test(NAME echoClient_test COMMAND echoClient_test)


#EchoServer_test
add_executable(echoServer_test EchoServer_test.cpp)
target_link_libraries(echoServer_test muduo_net)
add_test(NAME echoServer_test COMMAND echoServer_test)