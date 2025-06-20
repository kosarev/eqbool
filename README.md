# eqbool
Testing boolean expressions for equivalence.

```c++
#include "eqbool.h"

int main() {
    eqbool::eqbool_context eqbools;
    using eqbool::eqbool;

    eqbool eqfalse = eqbools.get_false();
    eqbool eqtrue = eqbools.get_true();

    // Constants are evaluated and eliminated right away.
    assert((eqfalse | ~eqfalse) == eqtrue);

    // Expressions get simplified on construction.
    eqbool a = eqbools.get("a");
    eqbool b = eqbools.get("b");
    assert((~b | ~eqbools.ifelse(a, b, ~b)) == (~a | ~b));

    // Identical, but differently spelled expressions are uniquified.
    eqbool c = eqbools.get("c");
    assert(((a | b) | c) == (a | (b | c)));

    // Speed is king, so simplifications that require deep traversals,
    // restructuring of existing nodes and increasing the diversity of
    // SAT clauses are intentionally omitted.
    eqbool d = eqbools.get("d");
    eqbool e1 = a & ((b | c) | (~a | ((~b | (d | ~c)) & (c | ~b))));
    eqbool e2 = a;
    assert(e1 != e2);
    assert(!eqbools.is_trivially_equiv(e1, e2));

    // The equivalence can still be established using SAT.
    assert(eqbools.is_equiv(e1, e2));

    // From there on, the expressions are considered identical.
    assert(eqbools.is_trivially_equiv(e1, e2));
}
```
[example.cpp](https://github.com/kosarev/eqbool/blob/master/example.cpp)
