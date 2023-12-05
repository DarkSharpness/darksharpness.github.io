#include <iostream>

// 模板类同理，这里就举一个模板函数的例子
template <typename _Tp>
void foo(_Tp *p) { p->~_Tp(); }

signed main() {
    struct B { int x; B() = default; };
    B *c = new B;   // 分配内存，什么都不干
    B *d = new B{}; // 分配内存，调用构造函数，初始化为 0
    B *e = new B(); // 分配内存，调用构造函数，初始化为 0
    std::cout << c->x << std::endl;
    std::cout << d->x << std::endl;
    std::cout << e->x << std::endl;

    void *buf = operator new(sizeof(std::string));      // raw memory
    std::string *p = ::new (buf) std::string("Hello");  // placement new

    std::cout << *p << std::endl;

    {
        using std::string;
        char buf[sizeof(std::string)];
        string *p = ::new (buf) string("Hello");
        foo(p);       // 通过模板函数调用析构函数
    }

    int x;
    foo(&x);         // 通过模板函数调用析构函数

    return 0;
}