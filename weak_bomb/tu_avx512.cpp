#include "header.hpp"
#include <numeric>

float NS::func_in_tu_avx512(TData& z) {
    prepare(z);
    return std::accumulate(z.begin(), z.end(), 0);
}
