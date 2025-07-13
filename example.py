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

    c = eqbool.Context()
    assert c.false | ~c.false == c.true

    # Terms can be strings, numbers and tuples.
    a = c.get('a')
    b = c.get('b')
    e = ~b | ~c.ifelse(a, b, ~b)
    assert e == ~a | ~b

    # Bool values can be verbalised as usual.
    print(e)

    c = c.get('c')
    assert (a | b) | c == a | (b | c)


if __name__ == "__main__":
    main()
