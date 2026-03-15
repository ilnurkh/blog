#include <cstdlib>
#include <unordered_map>
#include <iostream>

constexpr size_t ElemsToStore = 100'000;
constexpr size_t Seed = 27;

void InitialFill(auto& x) {
    srandom(Seed);
    for(size_t i = 0; i < ElemsToStore; ++i) {
        x[random()] = i;
    }
}

void ResetByOrders(auto& x) {
    size_t counter = 0;
    for(auto& p : x) {
        p.second = counter++;
    }
}

template<class T>
T RefillSimple(const T& x) {
    T res;
    for(auto& p : x) {
        res[p.first] = p.second;
    }
    return res;
}

template<class T>
T RefillReserved(const T& x) {
    T res;
    res.reserve(x.size());
    std::cout << "--- reserved to " << res.bucket_count() << std::endl;
    for(auto& p : x) {
        res[p.first] = p.second;
    }
    return res;
}

template<class T>
struct TGetMaxLoadFactor;


template<class K, class V>
struct TGetMaxLoadFactor<std::unordered_map<K, V>> {
    static float Get(const std::unordered_map<K, V>& x) {
        std::cerr << "x.max_load_factor()=" <<  x.max_load_factor() << std::endl;
        return x.max_load_factor();
    }
};

#ifdef ARCADIA
#include <util/generic/hash.h>
template<class K, class V>
struct TGetMaxLoadFactor<THashMap<K, V>> {
    static float Get(const THashMap<K, V>& ) {
        // just hand found value
        static_assert(ElemsToStore == 100'000, "recheck this");
        return 0.5;
    }
};
#endif

template<class T>
T RefillReserved2(const T& x) {
    T res;
    res.reserve(x.bucket_count() * TGetMaxLoadFactor<std::remove_cvref_t<decltype(x)>>::Get(x));
    std::cout << "--- reserved to " << res.bucket_count() << std::endl;
    for(auto& p : x) {
        res[p.first] = p.second;
    }
    return res;
}

size_t CalcMissOrders(const auto& x) {
    size_t res = 0;
    for(auto it = x.begin(); it != x.end();) {
        auto cur_value = it->second;
        // auto bckNum = x.bucket(it->first);
        ++it;
        if (it == x.end()) {
            break;
        }
        auto next_value = it->second;
        res += (next_value < cur_value);

        // if (next_value < cur_value && res < 10) {
        //     std::cout << "switch " << cur_value << "(" << bckNum << ") -> " << next_value << "(" << (x.bucket(it->first)) << ")" << std::endl;
        // }
    }

    // some debug print in THashTable; require some private->public changes in util/generic/hash.h and util/generic/hash_table.h
    // if (res < 1e3) {
    //     size_t prevBck = 1e6;
    //     size_t inBck = 0;
    //     for(auto it = x.begin(); it != x.end();) {
    //         auto cur_value = it->second;
    //         ++it;
    //         if (it == x.end()) {
    //             break;
    //         }
    //         size_t curBck = x.rep.bkt_num_key(it->first);
    //         if (prevBck == curBck) {
    //             inBck += 1;
    //         } else {
    //             inBck = 0;
    //         }
    //         auto next_value = it->second;
    //         size_t newBck = x.rep.bkt_num_key(it->first);
    //         if (next_value < cur_value) {
    //             std::cout << "MissOrder " << cur_value << " -> " << next_value << std::endl;
    //             std::cout << "--- cur bck" << curBck << "(num " << inBck << ") newbck=" << newBck << std::endl;
                
    //         }
    //     }
    // }
    return res;
}

template<class T>
void DoExp(std::string name) {
    T data;
    InitialFill(data);
    std::cout << name << ":filled size " << data.size() << std::endl;
    std::cout << name << " - with MissOrders " << CalcMissOrders(data) << std::endl;
    ResetByOrders(data);
    std::cout << name << " - resorted MissOrders (expect 0) " << CalcMissOrders(data) << std::endl;
    {
        T simpleRefill = RefillSimple(data);
        std::cout << name << " - simpleRefill MissOrders " << CalcMissOrders(simpleRefill) << std::endl;
        std::cout << name << " -- bucket count " << data.bucket_count() << " -> " << simpleRefill.bucket_count() << std::endl;
    }

    {
        T reservedRefill = RefillReserved(data);
        std::cout << name << " - reservedRefill MissOrders " << CalcMissOrders(reservedRefill) << std::endl;
        std::cout << name << " -- bucket count " << data.bucket_count() << " -> " << reservedRefill.bucket_count() << std::endl;
    }

    {
        T reservedRefill = RefillReserved2(data);
        std::cout << name << " - reservedRefill2 MissOrders " << CalcMissOrders(reservedRefill) << std::endl;
        std::cout << name << " -- bucket count " << data.bucket_count() << " -> " << reservedRefill.bucket_count() << std::endl;
    }

    {
        T reservedRefillRefill = RefillReserved2(RefillReserved2(data));
        std::cout << name << " - reservedRefillRefill MissOrders " << CalcMissOrders(reservedRefillRefill) << std::endl;
        std::cout << name << " -- bucket count " << data.bucket_count() << " -> " << reservedRefillRefill.bucket_count() << std::endl;
    }

}

int main() {
    std::cerr << "started" << std::endl;
    DoExp<std::unordered_map<int, size_t>>("unordered_map");
    #ifdef ARCADIA
    DoExp<THashMap<int, size_t>>("THashMap");
    #endif
    return 0;
}
