#include <cstdint>
#include <iostream>

struct int4_8 {
    int x0 : 4;
    int x1 : 4;
    long x2 : 4;
    unsigned x3 : 4;
    int x4 : 4;
    int x5 : 4;
    int x6 : 4;
    unsigned x7 : 4;
};

struct bit_pack_2 {
    int x       : 16; // 16 bit
    unsigned y  : 16; // OK, 和 x 在同一个 int 中

    int     z : 8;
    // 这里有 24 bit 的 padding,
    // 因为 uint8_t 只有 8 bit, 和 int 不一样
    uint8_t w : 8;
};

void test() {
    int4_8 x;
    x.x0 = 1;
    x.x1 = -3;
    std::cout << x.x0 << " " << x.x1 << std::endl;
}

static_assert(sizeof(int4_8) == 4, "int4_8 size is not 4 bytes");
static_assert(sizeof(bit_pack_2) == 12, "bit_pack_2 size is not 8 bytes");

signed main() {
    test();   
    return 0;
}