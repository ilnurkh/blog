#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <chrono>

std::mutex PrintLock;
std::atomic<bool> WaitFlag = false;

struct TBasicElem {
    static constexpr std::string_view Name = "noLock";
    uint8_t Data;

    void Lock() {}
    void UnLock() {}
};

#ifdef arcadia
#include <util/system/mutex.h>

struct TUtilMutex {
    static constexpr std::string_view Name = "arcadia TMutex";
    TMutex m;
    uint8_t Data;

    void Lock() {m.lock();}
    void UnLock() {m.unlock();}
};

#endif


struct TMutexElem {
    static constexpr std::string_view Name = "std::mutex";
    std::mutex m;
    uint8_t Data;

    void Lock() {m.lock();}
    void UnLock() {m.unlock();}
};

struct TMutexPtrElem {
    static constexpr std::string_view Name = "std::unique_ptr<std::mutex>";
    std::unique_ptr<std::mutex> m;
    uint8_t Data;

    TMutexPtrElem () {
        m = std::make_unique<std::mutex>();
    }
    void Lock() {m->lock();}
    void UnLock() {m->unlock();}
};

struct TAtomicFlagElem {
    static constexpr std::string_view Name = "std::atomic<bool>";
    std::atomic<bool> flag = false;
    uint8_t Data;

    void Lock() {
        while(true) {
            bool expected = false;
            if (flag.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
                break;
            }
        }
    }
    void UnLock() {flag.store(false, std::memory_order_release);}
};

struct TAtomicFlagWaitNotify {
    static constexpr std::string_view Name = "std::atomic<bool> wait + notify";
    std::atomic<bool> flag = false;
    uint8_t Data;

    void Lock() {
        while(true) {
            bool expected = false;
            if (flag.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
                break;
            } else {
                flag.wait(true, std::memory_order_acquire);
            }
        }
    }
    void UnLock() {
        flag.store(false, std::memory_order_release);
        flag.notify_all();
    }
};


struct TAtomicFlagPtrWaitNotify {
    static constexpr std::string_view Name = "std::atomic<bool> ptr wait + notify";
    std::unique_ptr<std::atomic<bool>> flag = std::make_unique<std::atomic<bool>>(false);
    uint8_t Data;

    void Lock() {
        while(true) {
            bool expected = false;
            if (flag->compare_exchange_weak(expected, true, std::memory_order_acquire)) {
                break;
            } else {
                flag->wait(true, std::memory_order_acquire);
            }
        }
    }
    void UnLock() {
        flag->store(false, std::memory_order_release);
        flag->notify_all();
    }
};

constexpr size_t AlignmentShift = 1024;
constexpr size_t CacheLineSize = 64;

template<class T>
struct TDataHolder {
    using TElem = T;
    char* Data;
    TElem* EffectiveDataPtr = nullptr;
    TElem* EffectiveEndPtr = nullptr;

    ~TDataHolder() {
        size_t accum = 0;
        for(auto ptr = EffectiveDataPtr; ptr != EffectiveEndPtr; ++ptr) {
            accum += ptr->Data;
        }

        // just to be sure that modifications of Data are visible outside
        std::cout << "checksum " << accum << std::endl;
        delete[] Data;
    }
    TDataHolder(float mbs) {
        std::srand(2026);

        const size_t recommendedSizeBytes = mbs * 1024 * 1024 + AlignmentShift;

        Data = new char[recommendedSizeBytes];
        EffectiveDataPtr = (decltype(EffectiveDataPtr))(size_t(Data) / AlignmentShift * AlignmentShift + AlignmentShift);
        EffectiveEndPtr = EffectiveDataPtr;
        while(size_t(EffectiveEndPtr + 1) < size_t(Data + recommendedSizeBytes)) {
            EffectiveEndPtr += 1;
        }

        assert(size_t(EffectiveDataPtr) / AlignmentShift * AlignmentShift == size_t(EffectiveDataPtr));
        std::cout << "\n\ninit " << TElem::Name << std::endl;
        std::cout << "- sizeof " << sizeof(TElem) << std::endl;
        std::cout << "- alignof " << alignof(TElem) << std::endl;
        std::cout << "- pool size is " << (EffectiveEndPtr - EffectiveDataPtr) / 1024. / 1024 << "mb elems " << std::endl;
        std::cout << "- it is " << (size_t(EffectiveEndPtr) - size_t(EffectiveDataPtr)) / 1024. / 1024 << " mbs capacity " << std::endl;

        for(auto ptr = EffectiveDataPtr; ptr != EffectiveEndPtr; ++ptr) {
            int rand = std::rand();
            new(ptr) TElem;
            ptr->Data =  rand % std::numeric_limits<std::remove_reference_t<decltype(ptr->Data)>>::max();
        }
    }

    size_t DoAction(size_t window, size_t shift, size_t subelems, std::string_view title, bool forward = true) {
        assert(EffectiveDataPtr + 1 == (TElem*)( size_t(EffectiveDataPtr) + sizeof(TElem)));
        while(WaitFlag.load()) {}
        size_t actionsDone = 0;
        auto actionsStarted = std::chrono::high_resolution_clock::now();
        for(size_t index = shift;  int64_t(index) < EffectiveEndPtr - EffectiveDataPtr; index += window) {
            for(size_t j = 0; j < subelems; ++j) {
                TElem& current = forward ? EffectiveDataPtr[index + j] : EffectiveDataPtr[(EffectiveEndPtr - EffectiveDataPtr) - index - j - 1];
                current.Lock();
                current.Data = current.Data * current.Data;
                current.UnLock();
                actionsDone += 1;
            }
        }
        auto actionsFinished = std::chrono::high_resolution_clock::now();

        {
            std::lock_guard g(PrintLock);
            std::cout << TElem::Name << ":" << title
                << "\n  window=" << window << "(bytes=" << sizeof(TElem) * window << ")"
                    << " shift=" << shift << " (bytes " << sizeof(TElem) * shift << ")"
                    << " subelems=" << subelems << " (bytes " << sizeof(TElem) * subelems << ")"
                << "\n -- done " << actionsDone / 1024. / 1024 
                << "mb actions, in " << (actionsFinished - actionsStarted).count() / 1e6 << "mln ticks"
                << "\n -- actions cost is " << (actionsFinished - actionsStarted).count() / (actionsDone + 0.0) << " ticks"
                << std::endl;
        }

        return actionsDone;
    }
};

int main() {
    TDataHolder<TAtomicFlagPtrWaitNotify> atomicWaitFlagPtrDataPool(128);
    {
        atomicWaitFlagPtrDataPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            atomicWaitFlagPtrDataPool.DoAction(2, 0, 1, "each second; t1");
        });
        std::thread t2([&]() {
            atomicWaitFlagPtrDataPool.DoAction(2, 1, 1, "each second; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TAtomicFlagPtrWaitNotify) * 2;
        atomicWaitFlagPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            atomicWaitFlagPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline; t1");
        });
        std::thread t2([&]() {
            atomicWaitFlagPtrDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TAtomicFlagPtrWaitNotify) * 2;
        atomicWaitFlagPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter;main thread", false);
        WaitFlag = true;
        std::thread t1([&]() {
            atomicWaitFlagPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter; t1");
        });
        std::thread t2([&]() {
            atomicWaitFlagPtrDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline different sides iter; t2", false);
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    TDataHolder<TAtomicFlagWaitNotify> atomicWaitFlagDataPool(128);
    {
        atomicWaitFlagDataPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            atomicWaitFlagDataPool.DoAction(2, 0, 1, "each second; t1");
        });
        std::thread t2([&]() {
            atomicWaitFlagDataPool.DoAction(2, 1, 1, "each second; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TAtomicFlagWaitNotify) * 2;
        atomicWaitFlagDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            atomicWaitFlagDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline; t1");
        });
        std::thread t2([&]() {
            atomicWaitFlagDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TAtomicFlagWaitNotify) * 2;
        atomicWaitFlagDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter;main thread", false);
        WaitFlag = true;
        std::thread t1([&]() {
            atomicWaitFlagDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter; t1");
        });
        std::thread t2([&]() {
            atomicWaitFlagDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline different sides iter; t2", false);
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    TDataHolder<TAtomicFlagElem> atomicFlagDataPool(128);
    {
        atomicFlagDataPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            atomicFlagDataPool.DoAction(2, 0, 1, "each second; t1");
        });
        std::thread t2([&]() {
            atomicFlagDataPool.DoAction(2, 1, 1, "each second; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TAtomicFlagElem) * 2;
        atomicFlagDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            atomicFlagDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline; t1");
        });
        std::thread t2([&]() {
            atomicFlagDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    TDataHolder<TMutexPtrElem> mutexPtrDataPool(128);
    {
        mutexPtrDataPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            mutexPtrDataPool.DoAction(2, 0, 1, "each second; t1");
        });
        std::thread t2([&]() {
            mutexPtrDataPool.DoAction(2, 1, 1, "each second; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TMutexPtrElem) * 2;
        mutexPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            mutexPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline t1");
        });
        std::thread t2([&]() {
            mutexPtrDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TMutexPtrElem) * 2;
        mutexPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter;main thread", false);
        WaitFlag = true;
        std::thread t1([&]() {
            mutexPtrDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter; t1");
        });
        std::thread t2([&]() {
            mutexPtrDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline different sides iter; t2", false);
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    #ifdef arcadia
    TDataHolder<TUtilMutex> utilMutexPool(128);
    {
        utilMutexPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            utilMutexPool.DoAction(2, 0, 1, "each second; t1");
        });
        std::thread t2([&]() {
            utilMutexPool.DoAction(2, 1, 1, "each second; t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = CacheLineSize / sizeof(TUtilMutex) * 2;
        utilMutexPool.DoAction(window_size, 0, 1, "nonintersected cacheline;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            utilMutexPool.DoAction(window_size, 0, 1, "nonintersected cacheline t1");
        });
        std::thread t2([&]() {
            utilMutexPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    #endif


    TDataHolder<TMutexElem> mutexDataPool(128);
    {
        mutexDataPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            mutexDataPool.DoAction(2, 0, 1, "t1");
        });
        std::thread t2([&]() {
            mutexDataPool.DoAction(2, 1, 1, "t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        mutexDataPool.DoAction(4, 0, 1, "nonintersected cacheline;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            mutexDataPool.DoAction(4, 0, 1, "nonintersected cacheline t1");
        });
        std::thread t2([&]() {
            mutexDataPool.DoAction(4, 2, 1, "nonintersected cacheline t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        size_t window_size = 4;
        mutexDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter;main thread", false);
        WaitFlag = true;
        std::thread t1([&]() {
            mutexDataPool.DoAction(window_size, 0, 1, "nonintersected cacheline different sides iter; t1");
        });
        std::thread t2([&]() {
            mutexDataPool.DoAction(window_size, window_size / 2, 1, "nonintersected cacheline different sides iter; t2", false);
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    TDataHolder<TBasicElem> basicDataPool(128);

    {
        basicDataPool.DoAction(2, 0, 1, "each second;main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            basicDataPool.DoAction(2, 0, 1, "t1");
        });
        std::thread t2([&]() {
            basicDataPool.DoAction(2, 1, 1, "t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    {
        basicDataPool.DoAction(64, 0, 1, "nonintersected cacheline; main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            basicDataPool.DoAction(64, 0, 1, "nonintersected cacheline t1");
        });
        std::thread t2([&]() {
            basicDataPool.DoAction(64, 1, 1, "nonintersected cacheline t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        basicDataPool.DoAction(64, 0, 32, "32 in each 64; main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            basicDataPool.DoAction(64, 0, 32, "t1");
        });
        std::thread t2([&]() {
            basicDataPool.DoAction(64, 32, 32, "t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }
    {
        basicDataPool.DoAction(128, 0, 64, "64 in each 128; main thread");
        WaitFlag = true;
        std::thread t1([&]() {
            basicDataPool.DoAction(128, 0, 64, "t1");
        });
        std::thread t2([&]() {
            basicDataPool.DoAction(128, 64, 64, "t2");
        });
        WaitFlag = false;
        t1.join();
        t2.join();
    }

    return 0;
}
