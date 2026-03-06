#cpp
После прошлого поста мне захотелось посмотреть ещё какие-нибудь примеры, где компилятор смог или не смог бы заметить инвариант.
Решил посмотреть на рефкаунт - сможет ли компилятор избавиться от атомарных += 1 и -=1, захочет ли.

Если кратко - то не смог (Но интереснее другое, но об этом чуть позже)
```
void doInc(std::shared_ptr<int> x) {
    (*x) += 7;
}

void doInc2(std::shared_ptr<int> x) {
    std::shared_ptr<int> y = x;
    (*y) += 7;
}
```
doInc2 - конских размеров, куча проверок, и вызов "виртуального деструктора" (`call    qword ptr [rax + 16]` - освобождение управляющего блока, видимо, там)

Свапы - уже получше, но не идеально
```
void doInc3(std::shared_ptr<int> x) {
    std::shared_ptr<int> y = std::move(x);
    (*y) += 7;
    x = std::move(y);
}
```
получилось
```
doInc3(std::shared_ptr<int>):
        movups  xmm0, xmmword ptr [rdi]
        mov     rax, qword ptr [rdi]
        xorps   xmm1, xmm1
        movups  xmmword ptr [rdi], xmm1
        add     dword ptr [rax], 7
        movups  xmmword ptr [rdi], xmm0
        ret
```
Что мы тут не видим? Не видим сравнений и чего-то странного, хотя `xorps   xmm1, xmm1` (и следующий mov полученных 128 битного нуля) - говорит что обнуление из move-а было скомпилено.

А теперь к тому что удивило - отсутствие проверок - не обнулили ли мы рефкаунт x в 1-ой и 3-ей функциях. И вот "в столько лет я узнал" - что по calling-convention деструктор передаваемых по значению аргументов вызывает вызывающая сторона.

Для проверки посмотрел что получается в таком случае
```
struct TWithDestruct {
    struct TState;
    std::unique_ptr<TState> State; 

    TWithDestruct();
    TWithDestruct(const TWithDestruct&);
    ~TWithDestruct();
};

void someAction(TWithDestruct);
TWithDestruct create();

void createAndDo() {
    TWithDestruct x = create();
    someAction(x);
}
```

В createAndDo можно увидеть два запуска деструкторов. И если задуматься - такое решение достаточно логично.
Например
```
call(calcParam1(), calcParam2())
```

если мы, например, уже посчитали calcParam1, а calcParam2 кинуло исключение - то надо бы первый аргумент уничтожить. Получается, если мы не дошли до вызова функции - нам придётся вызывать уничтожение. Если за деструктор отвечает вызывающая сторона, то можно легко объединить основной случай и unhappy-path.


[годболт](https://godbolt.org/z/7EGjo54br)



```
#include <memory>

void doInc(std::shared_ptr<int> x) {
    (*x) += 7;
}


void doInc2(std::shared_ptr<int> x) {
    std::shared_ptr<int> y = x;
    (*y) += 7;
}


void doInc3(std::shared_ptr<int> x) {
    std::shared_ptr<int> y = std::move(x);
    (*y) += 7;
    x = std::move(y);
}


void doIncCall(std::shared_ptr<int> x) {
    doInc(x);
}


struct TWithDestruct {
    struct TState;
    std::unique_ptr<TState> State; 

    TWithDestruct();
    TWithDestruct(const TWithDestruct&);
    ~TWithDestruct();
};

void someAction(TWithDestruct);
TWithDestruct create();

void createAndDo() {
    TWithDestruct x = create();
    someAction(x);
}
```
