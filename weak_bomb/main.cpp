#include "header.hpp"

int main(int argc, const char* argv[]) {
    std::cout << "start" << std::endl;
    std::cout << NS::func_with_choose(false) << std::endl;
    std::cout << "finish" << std::endl;

    return 0;
}
