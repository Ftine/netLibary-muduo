include(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if(NOT HAVE_ACCEPT4)
    set_source_files_properties(SocketsOps.cc PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()


set(net_SRCS
        Timer.cpp
        Channel.cpp
        TimerQueue.cpp
        EventLoop.cpp
        Poller.cpp
        Socket.cpp
        SocketsOpts.cpp
        InetAddress.cpp
        poller/DefaultPoller.cpp
        poller/EPollPoller.cpp
        poller/PollPoller.cpp
        EventLoopThread.cpp
        EventLoopThreadPool.cpp
        Buffer.cpp
        Acceptor.cpp
        TcpConnection.cpp
        TcpServer.cpp
        Connector.cpp
        TcpClient.cpp)
add_library(muduo_net ${net_SRCS})
target_link_libraries(muduo_net muduo_base)

install(TARGETS muduo_net DESTINATION lib)

set(HEADERS
        Buffer.h
        Callbacks.h
        Channel.h
        Endian.h
        EventLoop.h
        EventLoopThread.h
        EventLoopThreadPool.h
        InetAddress.h
        TcpClient.h
        TcpConnection.h
        TcpServer.h
        TimerId.h
        )
install(FILES ${HEADERS} DESTINATION include/muduo/net)

add_subdirectory(http)
add_subdirectory(test)
