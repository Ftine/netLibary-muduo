//
// Created by ftion on 2023/5/18.
//

#ifndef MUDUO_BASE_NONCOPYABLE_H
#define MUDUO_BASE_NONCOPYABLE_H

namespace muduo
{
    /*
     * 它是一个基类（base class）并且具有四个成员函数：
     * 一个拷贝构造函数和一个拷贝赋值运算符，它们都被显式地删除（delete）；
     * 一个默认构造函数和一个默认析构函数，它们都被标记为 = default。
     * noncopyable类的构造和析构函数被定义为 protected，这意味着只有派生类（derived class）可以访问它。
     * 该类的名称表明它的目的是作为一个不可复制的（noncopyable）类的基类，即它的派生类不能被复制（copy）。
     *
     * 拷贝构造函数和拷贝赋值运算符被显式地删除，表示禁止通过拷贝方式复制该类的实例。
     * 这可以防止在不期望的地方出现拷贝行为，从而提高代码的安全性和可靠性。
     */
    class noncopyable
    {
    public:
        // 拷贝构造函数
        noncopyable(const noncopyable&) = delete;
        // 拷贝赋值运算符
        void operator=(const noncopyable&) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };

}  // namespace muduo

#endif //MUDUO_BASE_NONCOPYABLE_H
