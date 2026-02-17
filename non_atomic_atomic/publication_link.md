this code is used in publication: TODO
publication content: 

Давайте обманем атомик. Вот две функции - можно ли используя только их увидеть некорректное состояние атомика?

```
void write(std::atomic<int64_t>& x, int64_t v) {
    x.store(v, std::memory_order_seq_cst);
}
int64_t read(std::atomic<int64_t>& x) {
    return x.load(std::memory_order_seq_cst);
}
```

Пишем тест:
- 1-ый тред упорно пишет -1 в атомик (то есть все биты равны единице)
- 2-ой тред пишет 2 (то есть ровно 1 бит - второй младший равен единице)
- 3-ий тред читает и стопает программу, если прочитает нечётное отрицательное число

```

void test(std::atomic<int64_t>& x) {
    constexpr size_t ITERS = 100'000'000;
    std::thread writePart1([&x]() {
        for(size_t i = 0; i < ITERS; ++i) {
            write(x, -1);
        }
    });
    std::thread writePart2([&x]() {
        for(size_t i = 0; i < ITERS; ++i) {
            write(x, 2);
        }
    });
    std::thread reader([&x]() {
        for(size_t i = 0; i < ITERS; ++i) {
            int64_t r = read(x);
            if ((r & 1) == 0 && r < 0) {
                std::cout << "found non atomic behaviour, iter=" << i/1e6 << "m" << std::endl;
                exit(0);
                return;
            }
        }
    });
    writePart1.join();
    writePart2.join();
    reader.join();
    std::cout << "finish, not found non atomic behaviour" << std::endl;
}
```

И, с помощью легко пропускаемой ошибки, эта программа у меня стабильно падает где-то за 7-10млн итераций чтения
```
+ clang++ -std=c++20 main.cpp -o nonatomic.exe -Wall -O2 -DNDEBUG
+ ./nonatomic.exe
found non atomic behaviour, iter=8.15546m
```

и вот оно - давайте посмотрим как мы обманули атомик
```
int main(int argc, const char* _ []) {
    constexpr size_t alignment = 4096;
    char* buf [alignment * 3];
    size_t ptr = size_t(buf) / alignment * alignment + alignment - 4;
    std::atomic<int64_t>* nonAligned = new((void*)ptr) std::atomic<int64_t>(0);
    std::atomic<int64_t> aligned;
    std::atomic<int64_t>& x = (argc == 1) ? *nonAligned : aligned;

    test(x);
    return 0;
}
```

Если не совершать ошибку (запустится с любым аргументом) то не промежуточного стейта мы не увидим
```
+ ./nonatomic.exe aligned
finish, not found non atomic behaviour
```

Думаю уже понятно что происходит - мы располагаем наш атомик на 8 байт по середине двух страниц.
В частности - мы располагаем старшие и младшие 4 биты в разных кеш-линиях
А алгоритмы синхронизации в процессоре между ядрами работают только в рамках кеш-линий.

Что же конкретное мы нарушили? Мы нарушили выравнивание объекта. `alignof(std::atomic<int64_t>) == 8`, а нарушение алайнмента - UB.
Как же посадить такую ошибку? Я точно видел такую ошибку в районе попытки реалзивоывать что-то вроде арен или memory-pool-ов.
Выделяют память, выдают её кусками, а про выравнивание при выделении забыли (это встроенные new сделают как надо, ибо знают тип, а malloc или самописные функции получения буфера из мемори-пула, этого не делают)

Что ещё интересного можно заметить? Давайте поштырим в godbold
https://godbolt.org/z/j5jP8ozEb

1. сами операции чтения и записи не требуют выравнивания (мы не получаем sigill, хотя например с simd инструкциями это легко случается даже на x86)
2. изменение memory-order ничего не меняет (видимо, можно попытаться заявить, что в случае x86 memory-order аргументы прежде всего описывают ограничений перестановки инструкций компилятором, но не порождают дополнительных явных инструкций (сброса буферов чтения/записи например))
3. компиляция функции про чтение вообще ничего про атомарность не содержит, а вот запись использует спец функцию xchg, про которую и написано что оно будет реализовывать внутрипроцессорный алгоритм блокировок
