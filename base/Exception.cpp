//
// Created by fight on 2023/5/22.
//

#include "Exception.h"
#include "CurrentThread.h"

namespace muduo
{

    Exception::Exception(string msg)
            : message_(std::move(msg)),
              stack_(CurrentThread::stackTrace(/*demangle=*/false))
    {
    }

}  // namespace muduo
