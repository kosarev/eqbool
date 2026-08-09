#!/usr/bin/env python3

# Testing boolean expressions for equivalence.
# https://github.com/kosarev/eqbool
#
# Copyright (C) 2023-2026 Ivan Kosarev.
# mail@ivankosarev.com
#
# Published under the MIT license.

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

    # Bool objects compare by identity, for speed; established
    # equivalences are followed by the underlying node ids.
    assert e1 != e2
    assert e1.id == e2.id

    # Order terms are ordinary terms stating that one of two
    # given values comes before the other; the opposite order is
    # the negation of the same term.
    orders = eqbool.OrderContext(ctx)
    a_b, b_c, a_c = ctx.get('a<b'), ctx.get('b<c'), ctx.get('a<c')
    orders.register_order(a_b, 'a', 'b')
    orders.register_order(b_c, 'b', 'c')
    orders.register_order(a_c, 'a', 'c')

    # Orderings whose terms chain into a cycle are impossible.
    assert orders.is_never(a_b & b_c & ~a_c)
    assert orders.is_possible(a_b & b_c)

    # Under consistent orders, spelling out the ordering implied
    # by transitivity changes nothing...
    assert orders.is_equiv(a_b & b_c, a_b & b_c & a_c)

    # ...but as plain propositions the two expressions differ,
    # and the order context never confuses the two views.
    assert not ctx.is_equiv(a_b & b_c, a_b & b_c & a_c)


if __name__ == "__main__":
    main()
