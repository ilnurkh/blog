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


