#include <bits/stdc++.h>
// #include <Dark/inout>

struct A {
    friend bool operator == (const A &,const A &) {
        std::cout << "Equal";
        return false;
    }

    friend auto operator <=> (const A &,const A &) {
        std::cout << "3-way";
        return 0 <=> 0;
    }
};

struct conless {
    A x;

    // friend bool operator == (const conless &,const conless &) = default;
    friend auto operator <=> (const conless &,const conless &) = default;
};



signed main() {
    std::cout << std::format("{0:_^10s}\n{0:_<10s}\n{0:_>10s}\n","yyu");
    std::cout << std::format("{:.3s}\n{:.3f}\n","yyuyyu",1.0);
    return 0;
}
