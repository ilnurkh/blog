#include <cassert>
#include <cstddef>
#include <iostream>

void update1(int64_t& x) {
    x += 1;
}
void update2(int64_t& x) {
    x -= 1;
}
int main(int argc, const char* argv[]) {
    assert(argc == 5);
    int64_t iters = atoll(argv[1]);
    int64_t step = atoll(argv[2]);
    int64_t cmp = atoll(argv[3]);
    int64_t p = atoll(argv[4]);

    int64_t sum = 0;
    int64_t sum1 = 0;
    int64_t value = 0;
    for(int64_t i = 0; i < iters; ++i) {
        value += step;
        value += 2 * step;
        value += 3 * step;
        value += 4 * step;
        value += 5 * step;
        value /= 2;
        // if (value % p <= cmp) {
            // update1(sum);
        // }
        // else {
            // update2(sum);
        // }
    }
    std::cout << sum << std::endl;

    return 0;
}
