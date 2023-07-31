//
// Created by ftion on 2023/5/18.
//

#ifndef MUDUO_BASE_COPYABLE_H
#define MUDUO_BASE_COPYABLE_H

namespace muduo
{
    //一个默认构造函数和一个默认析构函数，它们都被标记为 = default。
    // 这表示编译器应该生成默认的构造函数和析构函数，而不是显式地定义它们。
    // copyable类被定义为 protected，这意味着只有派生类（derived class）可以访问它。
    class copyable
    {
    protected:
        copyable() = default;
        ~copyable() = default;
    };

}  // namespace muduo


#endif //MUDUO_BASE_COPYABLE_H
