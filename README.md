# eqbool
Testing boolean expressions for equivalence.

eqbool is a C++ and Python rewrite of code originally developed as part
of a symbolic [gate-level Z80 simulator](https://github.com/kosarev/z80/tree/master/tests/z80sim) in pure Python, where
increasingly complex Boolean expressions describing gate states need to
be checked for equivalence.
[Z3](https://github.com/Z3Prover/z3) and several other existing libraries were tried and quickly proven
too slow for such use, so a custom solution had to be developed.

The library is specifically designed to reduce overall equivalence-check
times by simplifying expressions in ways that never increase the
diversity of [SAT](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem) clauses.

Where equivalence cannot be trivially established via simplifications,
eqbool uses the [CaDiCaL](https://github.com/arminbiere/cadical) solver.


```c++
#include "eqbool.h"

int main() {
    eqbool::term_set<std::string> terms;
    eqbool::eqbool_context eqbools(terms);
    using eqbool::eqbool;

    eqbool eqfalse = eqbools.get_false();
    eqbool eqtrue = eqbools.get_true();

    // Constants are evaluated and eliminated right away.
    assert((eqfalse | ~eqfalse) == eqtrue);

    // Expressions get simplified on construction.
    eqbool a = eqbools.get(terms.add("a"));
    eqbool b = eqbools.get(terms.add("b"));
    assert((~b | ~eqbools.ifelse(a, b, ~b)) == (~a | ~b));

    // Identical, but differently spelled expressions are uniquified.
    eqbool c = eqbools.get(terms.add("c"));
    assert(((a | b) | c) == (a | (b | c)));

    // Speed is king, so simplifications that require deep traversals,
    // restructuring of existing nodes and increasing the diversity of
    // SAT clauses are intentionally omitted.
    eqbool d = eqbools.get(terms.add("d"));
    eqbool e1 = a & ((b | c) | (~a | ((~b | (d | ~c)) & (c | ~b))));
    eqbool e2 = a;
    assert(!eqbools.is_trivially_equiv(e1, e2));

    // The equivalence can still be established using SAT.
    assert(eqbools.is_equiv(e1, e2));

    // From there on, the expressions are considered identical.
    assert(eqbools.is_trivially_equiv(e1, e2));

    // They then can be propagated to their simplest known forms.
    assert(e1 != e2);

    e1.propagate();
    e2.propagate();
    assert(e1 == e2);
}
```
[example.cpp](https://github.com/kosarev/eqbool/blob/master/example.cpp)


## In Python

```shell
pip install eqbool
```

```python
import eqbool


def main():
    # Directly created Bool objects have no associated value or context.
    assert eqbool.Bool().void

    ctx = eqbool.Context()
    assert ctx.false | ~ctx.false == ctx.true

    # Terms can be strings, numbers and tuples.
    a = ctx.get('a')
    b = ctx.get('b')
    e = ~b | ~ctx.ifelse(a, b, ~b)
    assert e == ~a | ~b

    # Bool values can be verbalised as usual.
    print(e)

    c = ctx.get('c')
    assert (a | b) | c == a | (b | c)

    # In the Python API, all values get propagated automatically, so
    # simple equality can be used to test for trivial equivalence.
    d = ctx.get('d')
    e1 = a & ((b | c) | (~a | ((~b | (d | ~c)) & (c | ~b))))
    e2 = a
    assert e1 != e2

    assert ctx.is_equiv(e1, e2)

    assert e1 == e2
```
[example.py](https://github.com/kosarev/eqbool/blob/master/example.py)
