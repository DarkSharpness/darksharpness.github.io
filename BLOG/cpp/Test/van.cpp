#include <bits/stdc++.h>

void test(int &x)        { std::cout << "Lvalue\n"; }
void test(const int &x)  { std::cout << "Const Lvalue\n"; }
void test(int &&x)       { std::cout << "Rvalue\n"; }
void test(const int &&x) { std::cout << "Const Rvalue\n"; }

template <class _Tp>
void func(_Tp &&var) {
    return test(std::forward <_Tp> (var));
}

signed main() {
    // int x = 10;
    // const int y = 1;
    int &&z = 2;
    test(z);
    test(std::move(z));
    // func(x);
    // func(y);
    // func(10);
    // func(std::move(x));
    // func(std::move(y));
    return 0;
}