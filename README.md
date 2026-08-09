# eqbool
Testing boolean expressions for equivalence.

eqbool is a C++ and Python rewrite of code originally developed as part
of a symbolic [gate-level Z80 simulator](https://github.com/kosarev/z80/tree/master/tests/z80sim) in pure Python, where
increasingly complex Boolean expressions representing gate states need to
be repeatedly checked for equivalence.
[Z3](https://github.com/Z3Prover/z3) and several other existing libraries were tried and quickly proven
too slow for such use, so a custom solution had to be developed.

The library is specifically designed to reduce overall equivalence-check
times by simplifying expressions in ways that never increase the
diversity of [SAT](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem) clauses.

Where equivalence cannot be trivially established via simplifications,
eqbool uses the [CaDiCaL](https://github.com/arminbiere/cadical) solver.
As the workload is very many small solves rather than a few hard ones,
the bundled CaDiCaL version is chosen by measuring on traces of real
runs, and is not necessarily the latest release.


```c++
#include "eqbool.h"

int main() {
    eqbool::term_set<std::string> terms;
    eqbool::eqbool_context eqbools(terms);
    eqbool::order_context orders(eqbools);
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

    // Order terms are ordinary terms stating that one of two
    // given values comes before the other; the opposite order
    // is the negation of the same term.
    eqbool a_b = eqbools.get(terms.add("a<b"));
    eqbool b_c = eqbools.get(terms.add("b<c"));
    eqbool a_c = eqbools.get(terms.add("a<c"));
    orders.register_order(a_b, terms.add("a"), terms.add("b"));
    orders.register_order(b_c, terms.add("b"), terms.add("c"));
    orders.register_order(a_c, terms.add("a"), terms.add("c"));

    // Orderings whose terms chain into a cycle are impossible.
    assert(orders.is_never(a_b & b_c & ~a_c));
    assert(orders.is_possible(a_b & b_c));

    // Under consistent orders, spelling out the ordering
    // implied by transitivity changes nothing...
    assert(orders.is_equiv(a_b & b_c, a_b & b_c & a_c));

    // ...but as plain propositions the two expressions differ,
    // and the order context never confuses the two views.
    assert(!eqbools.is_equiv(a_b & b_c, a_b & b_c & a_c));
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
    # Undefined Bool objects have no associated value or context.
    assert eqbool.Bool().is_undef

    ctx = eqbool.Context()
    assert ctx.false | ~ctx.false == ctx.true

    # Terms can be any hashable objects.
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
