#include <bits/stdc++.h>
// #include <Dark/inout>

// 注意，concept 也可以有默认参数，虽然一般没啥用
template <class T = void>
concept A = true;

// requires 最基础的用法，检测后面表达式是否为 constexpr true
template <class T>
requires (true && A <T>) || false
void func1(T) {
    throw;
    static_assert(false);
}

// 注意区分下面两个 requires 的区别
template <class T>
requires requires {
    func1(114514);
} && (sizeof(T) == 4)
struct tester {};

template <class T>
concept arithable = A <> && A <T> && requires(T x,T y) {
    x + y;
    x - y;
    x * y;
    x / y;
    typename tester <T>; // 检测类型是否存在
};

template <class T>
concept is_container = requires (T x) {
    x.size();  // 检测成员函数/成员变量
    x.empty();
    x.clear();
    typename T::iterator; // 检测类型是否存在
};

// 注意区分下面的两个 requires
template <class T>
concept is_custom = requires (T x){
    // 这里的 requires 是 requires 语句
    requires is_container <decltype(x.x)>;
};

struct custom_t { std::vector <int> x; };



// 输出 1 0 1
void print() {
    std::cout << arithable <int> << ' '
              << arithable <char> << ' '
              << is_custom <custom_t>;
}


template <class T>
void print_type(T) {
    std::cout << "others\n";
}

template <>
void print_type <bool> (bool) {
    std::cout << "bool\n";
}

template <class T = void>
concept nothing = true;

void printer() {
    constexpr auto __r = requires { sizeof(int); };
    print_type(__r);
    print_type(nothing <>);

}

signed main() {
    printer();
    print();
    // std::cout << arithable <int> << '\n';
    return 0;
}