//
// Created by ftion on 2023/5/18.
//

#ifndef MUDUO_BASE_TYPES_H
#define MUDUO_BASE_TYPES_H

#include <stdint.h>
#include <string.h>  // memset
#include <string>
#include <cstddef>

#ifndef NDEBUG
#include <assert.h>
#endif

namespace muduo{
    using std::string;

    /* inline关键字可以用于函数和类成员函数的声明和定义，
     * 用于指示编译器将函数代码作为内联代码插入到调用该函数的地方，
     * 而不是将其作为独立的函数进行调用。
     *
     * */
    inline void memZero(void* p, std::size_t n)
    {
        memset(p, 0, n);
    }

    /*
     * 使用`implicit_cast`作为`static_cast`或`const_cast`的安全版本，
     * 用于在类型层次结构中进行向上转换
     * （即将指向`Foo`的指针转换为指向`SuperclassOfFoo`的指针，或将指向`Foo`的指针转换为指向`Foo`的const指针）。
     * 当您使用`implicit_cast`时，编译器会检查转换是否安全。
     * 在许多情况下，使用明确的`implicit_cast`是必要的，因为C++要求精确的类型匹配，而不是将参数类型转换为目标类型。
     * 这种情况非常多，使用`implicit_cast`显式转换在这些情况下是必要的。
     */

    //向上:隐式类型转化 总是成功的
    template<typename To, typename From>
    inline To implicit_cast(From const &f)
    {
        return f;
    }

    /*
     * 当进行向上转型（即将类型为`Foo`的指针转换为类型为`SuperclassOfFoo`的指针）时，使用`implicit_cast`是可以的，因为向上转型总是成功的。
     * 但是，当进行向下转型（即将类型为`Foo`的指针转换为类型为`SubclassOfFoo`的指针）时，
     * 使用`static_cast`是不安全的，因为我们如何知道指针真正是`SubclassOfFoo`类型的？
     * 它可能是一个裸的`Foo`指针，或者是另一种类型的子类指针。因此，当进行向下转型时，应该使用这个宏定义。
     * 在调试模式下，我们使用`dynamic_cast`来再次检查向下转型是否合法（如果不合法，程序将终止）。在正常模式下，我们使用高效的`static_cast`。
     * 因此，在调试模式下测试转型的合法性非常重要！
     *
     * 这是代码中唯一应该使用`dynamic_cast`的地方。特别地，你不应该使用`dynamic_cast`来进行运行时类型识别（RTTI）的操作，例如下面的代码：
        ```cpp
        if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
        if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
        ```
     * 你应该设计代码的其他部分来避免这种情况的发生。
     */


    /*
     * 该函数的实现使用了implicit_cast函数，它将指针从From*类型转换为To*类型。
     *
     * 如果向下转型是不安全的，implicit_cast函数会导致编译器报错，因为不能将From*类型转换为To*类型。
     * 在这个函数中，if(false)语句的目的是为了触发编译器错误，以确保在进行不安全的向下转型时编译器会报错。
     * 当编译器执行到这个语句时，它会尝试执行implicit_cast<From*,To>(0)，但实际上这个语句永远不会被执行，因为if(false)永远为假。
     *
     * 在调试模式下，down_cast函数使用了dynamic_cast来检查向下转型是否合法。
     * 如果指针不是To*类型或者是NULL指针，则dynamic_cast会返回NULL，这时assert语句会触发一个断言错误。
     * 需要注意的是，这个检查只在调试模式下进行，并且需要开启运行时类型信息（RTTI）的支持。
     *
     * 最后，down_cast函数使用static_cast执行向下转型，并将转换后的指针返回。
     */

    template<typename To, typename From>     // use like this: down_cast<T*>(foo);
    inline To down_cast(From* f)                     // so we only accept pointers
    {
        // 如果向下转型是不安全的，implicit_cast函数会导致编译器报错
        if (false)
        {
            implicit_cast<From*, To>(0);
        }

        // 如果指针不是To*类型或者是NULL指针，则dynamic_cast会返回NULL，这时assert语句会触发一个断言错误。
        #if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
                assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
        #endif
        return static_cast<To>(f);
    }


}


#endif //MUDUO_BASE_TYPES_H
