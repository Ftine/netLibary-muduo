//
// Created by ftion on 2023/5/25.
//

#ifndef MUDUO_BASE_LOGGER_H
#define MUDUO_BASE_LOGGER_H

#include "LogStream.h"
#include "Timestamp.h"
#include "TimeZone.h"

namespace muduo{
    class TimesZone;

    /*
     * 提供设置日志级别和输出目的地的静态方法，使用LogStream的<<操作符将文件名，日志级别，时间等信息格式好提前写入LogBuffer中。
     * 日志默认级别Logger::INFO，默认输出到stdout。
     *
     * Logger负责全局的日志级别、输出目的地设置（静态成员函数），实际的数据流处理由Impl内部类实现。
     * Imp的成员变量LogSteam对象是实际的缓存处理对象，包含了日志信息的加工，通过Logger的stream()函数取得实现各种日志宏功能。
     * 当Logger对象析构时，将LogStream的日志数据flush到输出目的地，默认是stdout。
     */
    class Logger{
    public:
        enum LogLevel{TRACE,DEBUG,INFO,WARN,ERROR,FATAL,NUM_LOG_LEVELS,};

        // 编译器计算源文件名
        class SourceFile
        {
        public:
            template<int N>
            SourceFile(const char (&arr)[N])
                    : data_(arr),
                      size_(N-1)
            {
                const char* slash = strrchr(data_, '/'); // builtin function
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }
            explicit SourceFile(const char* filename)
                    : data_(filename)
            {
                const char* slash = strrchr(filename, '/');
                if (slash)
                {
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }

            const char* data_;
            int size_;
        };

        // 构造函数  析构函数 file 文件路径
        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char* func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        // 用于日志宏
        LogStream& stream() { return impl_.stream_; }

        // 全局方法，设置日志级别、flush输出目的地、日志时区等
        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);

        typedef void (*OutputFunc)(const char* msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc); // 默认 fwrite 到 stdout
        static void setFlush(FlushFunc);   // 默认 fflush 到 stdout
        static void setTimeZone(const TimeZone& tz);   // 默认 GMT


    private:

        // 私有类、实现--日志消息缓冲处理

        /*
         *（1）构造函数初始化各成员变量，往LogStream中写入格式化时间字符串、线程id，日志级别
         *（2）往LogStream中写入日志消息（实际是Logger外部处理的）
         *（3）往LogStream中写入记录日志所在源文件名、行数（Logger析构调用）
         */


        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
            void formatTime();
            void finish();

            Timestamp time_;  // 日志时间戳
            LogStream stream_; // 日志缓存流
            LogLevel level_;   // 日志级别
            int line_;         // 当前记录日式宏的 源代码行数
            SourceFile basename_;  // 当前记录日式宏的 源代码名称
        };

        Impl impl_;
    };

    // 全局的日志级别，静态成员函数定义，静态成员函数实现
    extern Logger::LogLevel g_logLevel;

    inline Logger::LogLevel Logger::logLevel()
    {
        return g_logLevel;
    }

    // 日志宏
    #define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
      muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
    #define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
      muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
    #define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
      muduo::Logger(__FILE__, __LINE__).stream()
    #define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
    #define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
    #define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
    #define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
    #define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

    const char* strerror_tl(int savedErrno); // 错误记录

    #define CHECK_NOTNULL(val) \
      ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

    // A small helper for CHECK_NOTNULL().
    template <typename T>
    T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
    {
        if (ptr == NULL)
        {
            Logger(file, line, Logger::FATAL).stream() << names;
        }
        return ptr;
    }

} // namespace muduo

#endif //MUDUO_BASE_LOGGER_H
