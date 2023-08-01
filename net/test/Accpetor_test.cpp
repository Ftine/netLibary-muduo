//
// Created by fight on 2023/6/11.
//
#include "../EventLoop.h"
#include "../Acceptor.h"
#include "../InetAddress.h"
#include "../SocketsOpts.h"

using namespace muduo;
using namespace muduo::net;

void newConnection(int socketfd, const InetAddress& peerAddr){
    printf("newConnection(): accepted a new conn---- %s\n",peerAddr.toIpPort().c_str());
    ::write(socketfd,"How are you?\n",13);
    sockets::close(socketfd);
}

int main(){
    printf("main(): pid = %d \n", getpid());

    InetAddress listenAddr(9981);
    EventLoop loop ;

    Acceptor acceptor(&loop,listenAddr, true);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();

}