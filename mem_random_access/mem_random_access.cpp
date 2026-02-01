#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>
#include <chrono>

template<size_t ElemSize>
struct TDataHolder {
    std::vector<uint8_t> Data;
    uint8_t* EffectiveDataPtr = nullptr;
    size_t ElemsNum = 0;

    TDataHolder(float mbs) {
        std::srand(2026);

        const size_t recommendedSizeBytes = mbs * 1024 * 1024;
        ElemsNum = recommendedSizeBytes / ElemSize + 1;
        const size_t alignment = 1024 * 4;
        const size_t finalSize = ElemsNum * ElemSize + alignment;
        std::cout << "finalSize=" << finalSize / 1024 / 1024 << " mb" << std::endl;
        std::cout << "elemsnum=" << ElemsNum / 1e6 << " mln" << std::endl;

        Data.resize(finalSize);
        EffectiveDataPtr = (uint8_t*)( size_t(&Data.front()) / alignment * alignment + alignment );

        for(size_t elemId = 0; elemId < ElemsNum; ++elemId) {
            int rand = std::rand();
            for(size_t i = 0; i < ElemSize; ++i) {
                EffectiveDataPtr[ElemSize * elemId + i] =  rand % std::numeric_limits<uint8_t>::max();
            }
        }
    }

    size_t DoActions(const std::vector<size_t>& shifts, std::vector<size_t>& dst) const {
        for(size_t shiftId = 0; shiftId < shifts.size(); shiftId += 1) {
            int64_t localAccum = 0;
            const int64_t* ptr = (int64_t*)( EffectiveDataPtr + ElemSize * shifts[shiftId] );
            for(size_t i = 0; i < ElemSize / sizeof(*ptr); i += 2) {
                localAccum +=  ptr[i] * ptr[i];
            }
            dst[shiftId] = localAccum;
        }
        return std::accumulate(dst.begin(), dst.end(), 0);
    }
};

template<size_t elemSize>
void DoWork(float poolSizeMb) {
    TDataHolder<elemSize> dataHolder(poolSizeMb);
    uint32_t actionsNum = 1e4;

    std::srand(2027);

    size_t controlSum = 0;
    std::vector<size_t> stats(1000);
    std::vector<size_t> actions(actionsNum);
    std::vector<size_t> buf(actionsNum);
    for(size_t iter = 0; iter < stats.size(); ++iter) {
        for(auto& x : actions) {
            x = rand() % dataHolder.ElemsNum;
        }
        auto actionsStarted = std::chrono::high_resolution_clock::now();
        controlSum += dataHolder.DoActions(actions, buf);
        auto actionsFinished = std::chrono::high_resolution_clock::now();

        stats[iter] = (actionsFinished - actionsStarted).count();
    }

    std::cerr << "conrol sum = " << controlSum << std::endl;
    std::sort(stats.begin(), stats.end());

    std::cout << elemSize << ": Elapsed q50: " << stats[stats.size() / 2] / 1e3 << "k ticks" << std::endl;
    std::cout << elemSize << ": Elapsed q90: " << stats[stats.size() * 0.9] / 1e3 << "k ticks" << std::endl;

}

int main(int argc, const char* argv[]) {
    float poolSizeMb = 128;

    if (argc >= 2) {
        poolSizeMb = std::stof(argv[2 - 1]);
    }

    DoWork<48>(poolSizeMb);
    DoWork<64>(poolSizeMb);
    DoWork<48>(poolSizeMb);
    DoWork<64>(poolSizeMb);

    return 0;
}
