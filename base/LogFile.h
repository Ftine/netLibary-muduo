//
// Created by fight on 2023/5/27.
//

#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include "Mutex.h"
#include "Types.h"
#include <memory>

namespace muduo {

    // AppendFile类型成员变量是最终用于操作本地文件的类，
    // -------负责将日志数据写入本地文件，记录写入的日志总长度。
    /*
     * 由于写日志到缓冲中使用非加锁的fwrite函数，
     * 因此要求外部单线程处理或加锁，保证缓冲区日志不会发生交织混乱。
     */
    namespace FileUtil {
        class AppendFile;
    }

    //LogFIle类是muduo日志库后端的实际管理者，主要负责日志的滚动。
    class LogFile : noncopyable {

    public:
        LogFile(const string &basename,    //  日志文件名，默认保存在当前工作目录下
                off_t rollSize,            //  日志文件超过设定值进行roll
                bool threadSafe = true,    //  默认线程安全，使用互斥锁操作将消息写入缓冲区
                int flushInterval = 3,     //  flush刷新时间间隔
                int checkEveryN = 1024);   //  每1024次日志操作，检查一个是否刷新、是否roll
        ~LogFile();

        // LogFile::append()函数添加日志内容，实际是调用AppendFile::append()函数。
        // 但是，这里还设计是否使用互斥锁、日志滚动的策略判断
        void append(const char *logline, int len);

        void flush();

        /*
         * 滚动日志
         * 相当于重新生成日志文件，再向里面写数据
         */
        bool rollFile();

    private:
        void append_unlocked(const char *logline, int len);

        /*
         * 构造一个日志文件名
         * 日志名由基本名字+时间戳+主机名+进程id+加上“.log”后缀
         */
        static string getLogFileName(const string &basename, time_t *now);  // 获取roll时刻的文件名

        const string basename_;
        const off_t rollSize_;
        const int flushInterval_;
        const int checkEveryN_;

        int count_;

        std::unique_ptr<MutexLock> mutex_;             // 操作AppendFiles是否加锁
        time_t startOfPeriod_;                         // 用于标记同一天的时间戳(GMT的零点)
        time_t lastRoll_;                              // 上一次roll的时间戳
        time_t lastFlush_;                             // 上一次flush的时间戳
        std::unique_ptr<FileUtil::AppendFile> file_;

        const static int kRollPerSeconds_ = 60 * 60 * 24;  // 一天的秒数 86400

    };
}

#endif //MUDUO_BASE_LOGFILE_H
