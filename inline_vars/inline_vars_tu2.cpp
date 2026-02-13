#include "inline_vars.hpp"


void UseTU2() {
    VarInGlobalNs.Call("TU2");
    NS::VarInGlobalNsHeader.Call("TU2");
    VarInAnonNsHeader.Call("TU2");
    InlineFunction().Call("TU2");
}
