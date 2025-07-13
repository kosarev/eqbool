#!/usr/bin/env python3

# Testing boolean expressions for equivalence.
# https://github.com/kosarev/eqbool
#
# Copyright (C) 2023-2025 Ivan Kosarev.
# mail@ivankosarev.com
#
# Published under the MIT license.

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


if __name__ == "__main__":
    main()
