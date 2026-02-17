#include <atomic>
#include <cstdlib>
#include <thread>
#include <iostream>

void write(std::atomic<int64_t>& x, int64_t v) {
    x.store(v, std::memory_order_seq_cst);
}
int64_t read(std::atomic<int64_t>& x) {
    return x.load(std::memory_order_seq_cst);
}

void test(std::atomic<int64_t>& x) {
    constexpr size_t ITERS = 100'000'000;
    std::thread writePart1([&x]() {
        for(size_t i = 0; i < ITERS; ++i) {
            write(x, -1);
        }
    });
    std::thread writePart2([&x]() {
        for(size_t i = 0; i < ITERS; ++i) {
            write(x, 2);
        }
    });
    std::thread reader([&x]() {
        for(size_t i = 0; i < ITERS; ++i) {
            int64_t r = read(x);
            if ((r & 1) == 0 && r < 0) {
                std::cout << "found non atomic behaviour, iter=" << i/1e6 << "m" << std::endl;
                exit(0);
                return;
            }
        }
    });
    writePart1.join();
    writePart2.join();
    reader.join();
    std::cout << "finish, not found non atomic behaviour" << std::endl;
}

int main(int argc, const char* _ []) {
    constexpr size_t alignment = 4096;
    char* buf [alignment * 3];
    size_t ptr = size_t(buf) / alignment * alignment + alignment - 4;
    std::atomic<int64_t>* nonAligned = new((void*)ptr) std::atomic<int64_t>(0);
    std::atomic<int64_t> aligned;
    std::atomic<int64_t>& x = (argc == 1) ? *nonAligned : aligned;

    test(x);
    return 0;
}