#include "inline_vars.hpp"

#include <iostream>

TMyType::TMyType(const char* x) {
    Name = x;
    printf("construct TMyType %zu %s\n", size_t(this) % 1024, Name);
}
TMyType::~TMyType() {
    printf("destruct TMyType %zu %s\n", size_t(this) % 1024, Name);
}

void TMyType::Call(const char* x) {
    printf("call %s TMyType %zu %s\n", x, size_t(this) % 1024, Name);
}

int main() {
    std::cout << "start" << std::endl;

    UseTU1();
    UseTU2();

    std::cout << "finish" << std::endl;
    return 0;
}
