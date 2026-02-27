publication link: 

#cpp
ODR и weak-символы - источник внезапных и не приятных проблем. Например, можно посадить вот такую мину, при компиляции под разные наборы инструкций.

`header.hpp`:
```
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
```

А также имеется два .cpp файла. Первый - tu_avx512.cpp например такой
```
#include "header.hpp"
#include <numeric>

float NS::func_in_tu_avx512(TData& z) {
    prepare(z);
    return std::accumulate(z.begin(), z.end(), 0);
}
```
(а второй - tu_common.cpp  отличается только именем определяемой функции)

Компилируем каждый translation-unit таким способом
```
clang++ -std=c++20 -c tu_avx512.cpp -o tu_avx512.a -msse4.2 -mavx -mavx2 -mavx512f
clang++ -std=c++20 -c tu_common.cpp -o tu_common.a -msse4.2 -mavx -mavx2
```

Вызываем правильную функцию (не avx512, оно кажется редкость на консьюмерских процах, на `13th Gen Intel(R) Core(TM) i5-1340P` например его нет)
```
#include "header.hpp"

int main(int argc, const char* argv[]) {
    std::cout << "start" << std::endl;
    std::cout << NS::func_with_choose(false) << std::endl;
    std::cout << "finish" << std::endl;

    return 0;
}
```

и легко получаем sigill - в зависимости от того, как слинковали

```
clang++ -std=c++20 main.cpp tu_avx512.a tu_common.a -o weak_bomb.exe
./weak_bomb.exe 
start
13073 Illegal instruction     (core dumped) ./weak_bomb.exe
```

А вот так - не получаем
```
clang++ -std=c++20 main.cpp tu_common.a tu_avx512.a -o weak_bomb2.exe
./weak_bomb2.exe 
start
8128
finish
```
Inline функция prepare при линковке была в первом случае выбрана из сборки с разрешёнными avx512f инструкциями.

Данный пример немного натянутый, но любое использование шаблонов порождает кучу инстанциаций inline различных методов, стоит хоть где-то не повезти с тем что не случился inline, а был использован call, и реализация была создана в TU со спец флагами - можно попасть на то, что именно это реализация будет выбрана где-то в совершенно другом месте.
