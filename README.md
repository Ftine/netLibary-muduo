# netLibary-muduo

该网络库参照mudup网络库进行编写，部分内容有些许改动，另外在代码中增加许多的注释和基础解释。
项目描述：用C++实现了一个高性能的TCP网络库，其提供了简单易用的API，在解耦网络传输和具体业务代码的同时能够提供高并发的网络IO服务。


## 结构组成

- base基础文件

  > 负责构建一些网络所需要的基本头问题，例如异步的LOG日志、封装好的系统调用等

- net

  > 项目的核心文件，基于Reactor模式 实现 one loop per thread 的 网络库设计。具体设计相关的EventLoop等对象。
  >
  > 整体采用主从Reactor的模型实现。其中很重要的一点 one loop per thread。
  >
  > > 具体来说：
  > >
  > > 在新建一个TcpConnection是从EvenLoopThreadPool中选一个eventloopThread给新的连接用。
  > >
  > > 也就是说：
  > >
  > > TcpServer自己的EventLoop只用来接受新连接，而新线程会用其他的EventLoop来执行具体的I/O操作。


