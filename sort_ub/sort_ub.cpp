#include <cstdlib>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <experimental/random>


struct TComparator {
    int mode = 0;
    bool operator()(const int* a, const int* b) {
        // int64_t apos = &a - ptrsToSort.data();
        // int64_t bpos = &b - ptrsToSort.data();
        // std::cerr << "cmp " << *a << " vs " << *b << std::endl;
        if (*a < 0) {
            std::cerr << "mode = " << mode << std::endl;
            std::cerr << "out of range deref a=" << *a  << std::endl;
            exit(1);
        }
        if (*b < 0) {
            std::cerr << "mode = " << mode << std::endl;
            std::cerr << "out of range deref b=" << *b << std::endl;
            exit(1);
        }

        if (mode == 0) {
            return *a < *b;
        }
        if (mode == 1) {
            return *a <= *b;
        }
        if (mode == 2) {
            if (a == b) {
                return false;
            }
            return std::experimental::randint(0, 10) < 5;
        }
        if (mode == 3) {
            if (a == b) {
                return std::experimental::randint(0, 10) < 5;
            }
            return *a < *b;
        }
        return std::experimental::randint(0, 10) < 5;
    }
};

constexpr size_t LEN = 100;
int main(int argc, const char* argv[]) {
    TComparator comp;
    if (argc > 1 && argv[1]) {
        comp.mode = argv[1][0] - '0';
    }

    for(size_t iter = 0 ; iter < 10'000; ++ iter) {
        std::vector<int> negValues = {-2, -1, -10, -20};
        std::vector<int> values;
        // if (iter % 1000 == 0) {
        //     std::cout << "iter = " << iter << std::endl;
        // }
        for(size_t i = 0; i < LEN; ++ i) {
            values.push_back(i * i % 100);
        }
        std::vector<const int*> ptrsToSort;
        ptrsToSort.push_back(&negValues[0]);
        ptrsToSort.push_back(&negValues[1]);
        for(auto& x : values) {
            ptrsToSort.push_back(&x);
        }
        ptrsToSort.push_back(&negValues[2]);
        ptrsToSort.push_back(&negValues[3]);
        std::sort(ptrsToSort.begin() + 2, ptrsToSort.end() - 2, comp);
        // for(auto x : ptrsToSort) {
        //     std::cout << *x << " ";
        // }
        // std::cout << std::endl;
    }
    std::cerr << "finished mode=" << comp.mode   << std::endl;
    return 0;
}