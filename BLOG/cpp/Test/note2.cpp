#pragma GCC optimize(2)
#include <iostream>

int func(int &x) { 
    while(x) {
        --x;
        func(x);
    }
}
int wtf_is_that() {
    std::cout << "Surprise!\n";
    return 0;
}

int main() {
    int y = 10;
    std::cout << func(y) << ' ';
    std::cout << y << '\n';
    return 0;
}
