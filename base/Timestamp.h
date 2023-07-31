//
// Created by ftion on 2023/5/18.
//

#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H
#include "Types.h"
#include "copyable.h"
#include <boost/operators.hpp>
namespace muduo {

    /*
     * Timestamp类表示一个时间戳，用于记录时间的信息。
     * 它的主要作用是提供一种可靠的方式来表示时间，以便在程序中进行时间相关的操作。
     */
    class Timestamp : public muduo::copyable,
                      public boost::equality_comparable<Timestamp>,
                      public boost::less_than_comparable<Timestamp> {
    public:
        Timestamp() : microSecondsSinceEpoch_(0) {};

        //显示构造
        explicit Timestamp(int64_t microSecondsSinceEpochArg) :
                microSecondsSinceEpoch_(microSecondsSinceEpochArg) {};

        //交换时间戳
        void swap(Timestamp &that) {
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

        // 格式化时间戳的字符串
        string toString() const;

        string toFormattedString(bool showMicroseconds = true) const;

        bool valid() const { return microSecondsSinceEpoch_ > 0; } // 时间戳微秒数是否大于无效0

        // 内部使用, 获取时间戳、时间戳秒数
        int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

        time_t secondsSinceEpoch() const {
            return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
        }

        // 当前时间戳
        static Timestamp now();

        static Timestamp invalid() { return Timestamp(); }

        // 从time_t、微秒偏移得到时间戳
        static Timestamp fromUnixTime(time_t t) { return fromUnixTime(t, 0); }

        static Timestamp fromUnixTime(time_t t, int microseconds) {
            return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
        }

        static const int kMicroSecondsPerSecond = 1000 * 1000; // 1s的微秒数
    private:
        int64_t microSecondsSinceEpoch_; // 1s的微秒数

    };

    // 运算符重载 判断时间戳的大小
    inline bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    // 计算两个时间戳之间的时间差异 结果返回使 xx.xx s
    inline double timeDifference(Timestamp high, Timestamp low)
    {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }

    // 加时间 seconds是一个s为单位的小数
    inline Timestamp addTime(Timestamp timestamp, double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }

} // namespace muduo


#endif //MUDUO_BASE_TIMESTAMP_H
