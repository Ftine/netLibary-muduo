# 整体架构

>客户端发送请求，通过muduo库之后服务端收到的数据存放于Buffer中，
之后解析成HttpRequest请求对象，
再创建一个HttpResponse响应对象并格式化成Buffer返回给客户端。 
> 服务端解析客户端请求的Buffer使用HttpContext类。


## HttpRequest 类
- 发送端构造一个HttpRequest，调用成员函数设置请求头、请求体等。
- 调用成员函数请求头字段，使用 const char* start, const char* end 来传递一个字符串。

>主要是因为基于TCP的请求数据都保存在Buffer中，通过解析Buffer中数据进行传递HTTP报文信息。

## HttpResponse 类
- 构造一个HttpResponse，调用成员函数设置响应头部、响应体，调用appendToBuffer()格式化到Buffer中，回复给客户端。


## HttpContext 类
- 服务端接收客户请求通过HttpContext解析，解析后数据封装到HttpRequest中。