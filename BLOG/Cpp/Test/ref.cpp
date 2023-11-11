#include <iostream>
// #include <bits/stdc++.h>

int func() { return 0; }

void foo0() {
    int x = 0;
    int &y = x;
    // int &z = 0;
    // int &w = func();
    const int &z = 0;
    const int &w = func();
}


struct tester {
    tester() = default;
    tester(const tester &) {
        std::cout << "Copy constructor\n";
    }
    tester(tester &&) {
        std::cout << "Move constructor\n";
    }
    tester(const int &) {
        std::cout << "const int &constructor\n";
    }
    tester(int &&) {
        std::cout << "int &&constructor\n";
    }
};

tester say(int x) {
    return std::move(x);
}

signed main() {
    say(1);
    return 0;
}