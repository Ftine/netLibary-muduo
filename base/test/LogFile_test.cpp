#include "../LogFile.h"
#include "../Logging.h"

#include <unistd.h>

std::unique_ptr<muduo::LogFile> g_logFile;

void outputFunc(const char* msg, int len){
    g_logFile->append(msg, len);
}

void flushFunc(){
    g_logFile->flush();
}

int main(int argc, char* argv[])
{
    char name[256] = { '\0' };
    strncpy(name, argv[0], sizeof name - 1);

    // 日志文件最大200*1000字节，默认线程安全
    g_logFile.reset(new muduo::LogFile(::basename(name), 200*1000));

    //muduo::Logger::setOutput(outputFunc);
    //muduo::Logger::setFlush(flushFunc);

    // g_logFile 是全局变量，可以直接在lambda闭包中使用
    muduo::Logger::setOutput( [](const char* msg, int len) {
        g_logFile->append(msg, len);
    });
    muduo::Logger::setFlush(  []{ g_logFile->flush(); });

    muduo::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    for (int i = 0; i < 10000; ++i)
    {
        LOG_INFO << line << i;
        usleep(1000);
    }
}
