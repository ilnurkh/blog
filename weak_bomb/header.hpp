#include <iostream>
#include <array>

namespace NS {

using TData = std::array<float, 128>;

float func_in_tu_common(TData& z);
float func_in_tu_avx512(TData& z);

inline void prepare(TData& z) {
    for(size_t i = 0; i < z.size(); ++i) {
        z[i] = i;
    }
}

inline float func_with_choose(bool haveAvx) {
    TData z;
    if (haveAvx) {
        return func_in_tu_avx512(z);
    }
    return func_in_tu_common(z);
}

}
