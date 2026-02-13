this code is used in publication: TODO
publication content:

#cpp

Есть достаточно редко используемая относительно свежая фича - inline переменные.
Вот нужно завести какой-то объект, константный глобальный. И как же лень писать его объявление, а определение в какой-то .cpp приземлять. Там ещё и синтаксис такой дурацкий.

И вот казалось бы - ура, можно теперь написать что-то вида

```
inline const std::vector<int> VeryImportantList = {1, 2, 3};
```

в самом удобном .h файле и радоваться жизни.

Что вообще эта штука значит? `inline`, также как в случае с функциями, лишь объявляет символ слабым (`weak`) - то есть говорит линтеру "оставь 1 любую копию из всех одноимённых символов". 

Легко совершить вот такую ошибку (кстати - это не хорошо как с переменными, так и с функциями)

```
namespace {
inline const std::vector<int> VeryImportantList = {1, 2, 3};
}
```
 - объявить переменную в анонимном неймспейсе. Сочетаем несочетаемое: namespace объявляет видимость локально на translation-unit (на cpp файл), и это перекрывает логику от inline атрибута


Покажу на примере простой тестовой программы 
```
# inline_vars.hpp
#pragma once

struct TMyType {
    const char* Name;
    TMyType(const char* x);
    ~TMyType();
    void Call(const char* x);
};

inline TMyType VarInGlobalNs = {"global_ns"};
namespace NS {
    inline TMyType VarInGlobalNsHeader = {"in_ns"};
}
namespace {
    inline TMyType VarInAnonNsHeader = {"in_anon_ns"};
}
inline TMyType& InlineFunction() {
    static TMyType inFunc = {"in_func"};
    return inFunc;
}

void UseTU1();
void UseTU2();

# inline_vars_tu1.cpp
void UseTU1() {
    VarInGlobalNs.Call("TU1");
    NS::VarInGlobalNsHeader.Call("TU1");
    VarInAnonNsHeader.Call("TU1");
    InlineFunction().Call("TU1");
}
# UseTU2 - такой же как TU1, только замена строк на "TU2"


# main.cpp
#include "inline_vars.hpp"

#include <iostream>

TMyType::TMyType(const char* x) {
    Name = x;
    printf("construct TMyType %zu %s\n", size_t(this) % 1024, Name);
}
TMyType::~TMyType() {
    printf("destruct TMyType %zu %s\n", size_t(this) % 1024, Name);
}

void TMyType::Call(const char* x) {
    printf("call %s TMyType %zu %s\n", x, size_t(this) % 1024, Name);
}

int main() {
    std::cout << "start" << std::endl;

    UseTU1();
    UseTU2();

    std::cout << "finish" << std::endl;
    return 0;
}
```

В программе создаём тип с логированием конструктора, деструктора несколькими способами - inline переменая, inline переменая в анонимном неймспейсе, в неаноимном нейспейсе, а также используя static переменную внутри inline функции

лог программы
```
construct TMyType 144 global_ns
construct TMyType 160 in_ns
construct TMyType 176 in_anon_ns
construct TMyType 192 in_anon_ns
construct TMyType 216 in_anon_ns
start
call TU1 TMyType 144 global_ns
call TU1 TMyType 160 in_ns
call TU1 TMyType 192 in_anon_ns
construct TMyType 200 in_func
call TU1 TMyType 200 in_func
call TU2 TMyType 144 global_ns
call TU2 TMyType 160 in_ns
call TU2 TMyType 216 in_anon_ns
call TU2 TMyType 200 in_func
finish
destruct TMyType 200 in_func
destruct TMyType 216 in_anon_ns
destruct TMyType 192 in_anon_ns
destruct TMyType 176 in_anon_ns
destruct TMyType 160 in_ns
destruct TMyType 144 global_ns
```

Что мы видим?
1. кроме объявления внутри функции - конструктор случается до старта main
2. вариант в анонимном нейсмпейсе был создан трижды с разными адресами
3. корректный порядок деструкторов (обратный порядку инициализации)

Интересное: попытки делать не printf, а stdout - сегфолтятся, так как эти глобальные объекты ещё не готовы на момент вывзова конструкторов статичных глобальных переменных.

