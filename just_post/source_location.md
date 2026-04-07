https://en.cppreference.com/w/cpp/utility/source_location.html

Коллега показал прикольную штуку - std::source_location (ещё в 20-ых плюсах добавили) - артефакт борьбы с макросами

```
void log(const std::string_view message,
         const std::source_location location =
               std::source_location::current())
```
как легко догадаться (или посмотреть полный пример по ссылке) - дефолтное значение в функцию вычислится в точке вызова 

> NOTE: это кстати вполне общая механика, компилятор не "предсоздаёт дефолтное значение функции", а добавляет его вычисление в точке вызова указанным в объявлении выражением. В примере (https://godbolt.org/z/Y1c847MzY) можно увидеть два вызова конструкторов TSomeObject

Сделано оно конечно скучно - `__builtin_source_location`, то есть нужна спец поддержка компилятора.

Последнее что хочется добавить - сравнение с TSourceLocation аркадийным (https://github.com/ytsaurus/ytsaurus/blob/dc3b9b0194c7104ca16af4099272d7f4e61c219f/util/system/src_location.h#L25). Тот хранит меньше инфы (не хранит десимволизированное имя функции и смещение в строке), что даже полезно - найти функцию по строке проблема не большая, а новые строки в бинарник добавляются (которые даже при `strip` не получится удалить). Более того - строки которые создаёт source_location - содержат подстановку шаблонных параметров (то есть полное имя инстанцированной шаблонной функции, если она шаблонная), что конечно может вызывать нежелательное раздутие бинаринка, если использовать повсеместно. К сожалению, не нашёл сходу как создать свой std::source_location, который бы не содержал часть полей (приватные поля нельзя поправить), но сделать свою обёртку вполне возможно - в примере (https://godbolt.org/z/rzGzqvE6b) с TSmallSourceLocation можно увидеть, что компилятор породил константу `void useLog1()`, а аналога для `useLog2` в выхлоп не добавил.



```
#include <iostream>
#include <source_location>
#include <string_view>
 
  using __bsl_ty  = decltype(__builtin_source_location());

struct TSmallSourceLocation {
    const char* _M_file_name;
    unsigned _M_line;

    const char* file_name() const {
        return _M_file_name;
    }
    unsigned line() const {
        return _M_line;
    }
};

consteval TSmallSourceLocation small_sl(
    __bsl_ty __ptr = __builtin_source_location())
{
    std::source_location tmp = std::source_location::current(__ptr);

    TSmallSourceLocation res;
    res._M_file_name = tmp.file_name();
    res._M_line = tmp.line();
    return res;
}

void log1(const std::string_view message,
         const std::source_location location =
               std::source_location::current())
{
    std::clog << "file: "
              << location.file_name() << '('
              << location.line() << ':'
              << location.column() << ") `"
              << location.function_name() << "`: "
              << message << '\n';
}

void log2(const std::string_view message,
         const TSmallSourceLocation location =
               small_sl())
{
    std::clog << "file: "
              << location.file_name() << '('
              << location.line() << ')'
            //   << location.column() << ") `"
            //   << location.function_name() << "`: "
              << message << '\n';
}
 
 
void useLog1()
{
    log1("Hello world!"); // line 25
}
void useLog2()
{
    log2("Hello world!"); // line 25
}

int main(int, char*[]) {
    useLog1();
    useLog2();
    return 0;
}
```
