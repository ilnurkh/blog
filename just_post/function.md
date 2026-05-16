#cpp

Прекрасное дополнение к лямбдам, появившееся вместе с ними в с++11, это std::function:
type-erasure обёртка, которая при конструировании запоминает конструктор, деструктор, копирование, ну и invoke.
Часто заменяет собой использование интерфейсов (виртуальных методов), особенно когда нужно было бы 1 только функцию заменить.

Вопрос - насколько компилятор хорошо справляется с разворачиванием std::function?

Рассмотрим функцию
```
int MakeAction(std::function<int(void)> f) {
    MarkerStart();
    int x = f();
    MarkerEnd();
    return x;
}
```
(а также объявим без определения MarkerStart, MarkerInnerActionInt, MarkerEnd чтобы по call директивам следить что дала компиляция)

Результат компиляции с лямбдой
```
void MakeActionLambda() {
    MakeAction([](){return MarkerInnerAction();});
}
```

оказался достаточно интересным. Если заменить `int MakeAction(std::function<int(void)> f)` на `template<class F> int MakeAction(F&& f) ` то компилятор просто сделает 3 call-а, то есть ожидаемым образом всё подставит.

А с std::function сложнее: будет и сконструирован объект std::function, и будет вызван его деструктор

Конструирование - заполнение полей (указателей на функции)
```
        push    rbx
        sub     rsp, 32
        xorps   xmm0, xmm0
        movaps  xmmword ptr [rsp], xmm0
        lea     rax, [rip + std::_Function_handler<int (), MakeActionLambda()::$_0>...]
        mov     qword ptr [rsp + 24], rax
        lea     rax, [rip + std::_Function_handler<int (), MakeActionLambda()::$_0>...]
        mov     qword ptr [rsp + 16], rax
```

Потом вызовы (+ прикапывание rax с результатом MarkerInnerAction для будущего return)
```
        call    MarkerStart()@PLT
        call    MarkerInnerAction()@PLT
        call    MarkerEnd()@PLT
        mov     rax, qword ptr [rsp + 16]
```

И деструктор
```
        test    rax, rax
        je      .LBB2_5
        mov     rdi, rsp
        mov     rsi, rdi
        mov     edx, 3
        call    rax
```

Сохранение тройки - это как раз выбор деструктора [1](https://blog.weghos.com/llvm/include/c++/12/bits/std_function.h.html#std::__destroy_functor) [2](https://blog.weghos.com/llvm/include/c++/12/bits/std_function.h.html#203)

Далее ещё идёт блок для разворачивания стека. Если MarkerStart и MarkerEnd указать как noexcept - то компилятор опять не будет "лишние" инструкции добавлять. Итого: в принципе компилятор может справляться с разворачиванием (инлайнингом) std::function (если видит его конструирование), но это явно добавляет ему сложностей (можно представить что необходимость как-то аккуратнее сохранять стек для его раскрутки, заставляет создавать объект, но вызывать деструктор который заведомо ничего не делает - выглядит как недооптимизация).

[годболт](https://godbolt.org/z/zxEef3ch8)

```
#include <functional>

void MarkerStart();
int MarkerAction();
int MarkerInnerAction();
int MarkerInnerActionInt(int);
void MarkerEnd();

#ifdef TEMPLATE
    template<class F>
    int MakeAction(F&& f) {
        MarkerStart();
        int x = f();
        MarkerEnd();
        return x;
    }
#else

    int MakeAction(std::function<int(void)> f) {
        MarkerStart();
        int x = f();
        MarkerEnd();
        return x;
    }
#endif


void MakeActionFunc() {
    MakeAction(&MarkerInnerAction);
}

void MakeActionLambda() {
    MakeAction([](){return MarkerInnerAction();});
}

void MakeActionLambda2() {
    MakeAction([](){return MarkerInnerActionInt(5);});
}
void MakeActionLambda4() {
    int x = 7;
    MakeAction([&x](){return MarkerInnerActionInt(x);});
}
void MakeActionLambda3() {
    int x = 17;
    MakeAction([x](){return MarkerInnerActionInt(x);});
}

int main() {}
```

```

MakeActionLambda():
        push    rbx
        sub     rsp, 32
        xorps   xmm0, xmm0
        movaps  xmmword ptr [rsp], xmm0
        lea     rax, [rip + std::_Function_handler<int (), MakeActionLambda()::$_0>::_M_invoke(std::_Any_data const&)]
        mov     qword ptr [rsp + 24], rax
        lea     rax, [rip + std::_Function_handler<int (), MakeActionLambda()::$_0>::_M_manager(std::_Any_data&, std::_Any_data const&, std::_Manager_operation)]
        mov     qword ptr [rsp + 16], rax
        call    MarkerStart()@PLT
        call    MarkerInnerAction()@PLT
        call    MarkerEnd()@PLT
        mov     rax, qword ptr [rsp + 16]
        test    rax, rax
        je      .LBB2_5
        mov     rdi, rsp
        mov     rsi, rdi
        mov     edx, 3
        call    rax
.LBB2_5:
        add     rsp, 32
        pop     rbx
        ret
        mov     rdi, rax
        call    __clang_call_terminate
        mov     rbx, rax
        mov     rax, qword ptr [rsp + 16]
        test    rax, rax
        je      .LBB2_8
        mov     rdi, rsp
        mov     rsi, rdi
        mov     edx, 3
        call    rax
.LBB2_8:
        mov     rdi, rbx
        call    _Unwind_Resume@PLT
        mov     rdi, rax
        call    __clang_call_terminate
```


--------

Небольшое дополнение к предыдущему посту: посмотрим как себя компилятор ведёт с интерфейсами.

Объявим интерфейс, объявим его реализацию, которая умеет взять лямбду и форварднуть её, ну и тип MakeAction сменим
```
struct ICallable {
    virtual ~ICallable() {}
    virtual int operator()() = 0;
};

template<class F>
struct TLambdaCallable : public ICallable {
    F Functor;
    TLambdaCallable(F&& f) : Functor(std::move(f))
    {}
    int operator()() final {
        return Functor();
    }
};


int MakeAction(ICallable&& f) {
    MarkerStart();
    int x = f();
    MarkerEnd();
    return x;
}
```

Примерно любые варианты определений (кстати, спасибо выводу шаблонных параметров классов из конструктора - выглядит это компактно)
```
void MakeActionFunc() {
    MakeAction(TLambdaCallable{&MarkerInnerAction});
}

void MakeActionLambda() {
    MakeAction(TLambdaCallable{[](){return MarkerInnerAction();}});
}
```

Компилятор оптимизирует в одинаковый результат - без всяких лишних действий, и никакого влияния `noexcept` тоже нет - хорошо и с ним и, без него. Видимо, стоит констатировать, что интерфейсы более понятная компилятору штука, чем std::function.

```
MakeActionLambda():
        push    rax
        call    MarkerStart()@PLT
        call    MarkerInnerAction()@PLT
        pop     rax
        jmp     MarkerEnd()@PLT
```

https://godbolt.org/z/GPnYcKEMc

--------------
