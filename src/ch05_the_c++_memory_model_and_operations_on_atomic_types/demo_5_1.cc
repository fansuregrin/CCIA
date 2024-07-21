#include <cstdio>
#include <string>

struct my_data {
    int i;
    double d;
    unsigned bf1:10;
    int bf2:25;
    // int bf3:0;  // zero-length bit field must be unnamed
    int :0;
    int bf4:9;
    int i2;
    char c1, c2;
    std::string s;
};

int main() {
    my_data x;
    printf("address of x.i: %p\n", &x.i);
    printf("address of x.d: %p\n", &x.d);
    printf("address of x.i2: %p\n", &x.i2);
    printf("address of x.c1: %p\n", &x.c1);
    printf("address of x.c2: %p\n", &x.c2);
    printf("address of x.s: %p\n", &x.s);

    x.bf2 = 0b1010111000101010010100111; // 0x15c54a7
    x.bf1 = 0b0000000001;
    x.bf4 = 0b101001100; // 0x14c
    printf("x.bf1: %x\n", x.bf1);
    printf("x.bf2: %x\n", x.bf2);
    printf("x.bf4: %x\n", x.bf4);
}