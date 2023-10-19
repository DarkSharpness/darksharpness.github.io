#include <iostream>
#include <format>
#include <thread>
// #include <bits/stdc++.h>

using namespace std::chrono_literals;

struct unsafe {
    unsafe(const char *) noexcept {
        std::cout << "Construct!\n";
    }
    unsafe(const unsafe &) noexcept {
        std::cout << "Copy!\n";
    }
    unsafe(unsafe &&) noexcept {
        std::cout << "Move!\n";
    }
    ~unsafe() noexcept {
        std::cout << "Destruct!\n";
    }
};

void func(const unsafe &str) {
    std::this_thread::sleep_for(2s);
    std::cout << "Working!" << '\n';
}

void test(const std::string &str) {
    std::this_thread::sleep_for(2s);
    std::cout << str << '\n';
}

template <class T>
void tmp(T &&x) {
    std::this_thread::sleep_for(2s);
    std::cout << "Template\n";
    std::cout << x << '\n';
}

void start(char x = 0) {
    char buf[] = "Hello";
    buf[0] = 'H' + x;
    std::thread t = std::thread(tmp <std::string &&>, "Hello");
    t.detach();
}

int stack_killer(int x) {
    int a[10];
    for(int i = 0 ; i < 10 ; ++i)
        a[0] = (a[i] = i * x);
    return a[0];
}

signed main() {
    start();
    stack_killer(2);
    std::this_thread::sleep_for(4s);
    return 0;
}