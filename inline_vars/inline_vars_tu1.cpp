#include "inline_vars.hpp"


void UseTU1() {
    VarInGlobalNs.Call("TU1");
    NS::VarInGlobalNsHeader.Call("TU1");
    VarInAnonNsHeader.Call("TU1");
    InlineFunction().Call("TU1");
}
