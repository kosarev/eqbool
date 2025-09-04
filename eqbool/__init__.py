# -*- coding: utf-8 -*-

#   Testing boolean expressions for equivalence.
#   https://github.com/kosarev/eqbool
#
#   Copyright (C) 2023-2025 Ivan Kosarev.
#   mail@ivankosarev.com
#
#   Published under the MIT license.


from ._eqbool import _Context
from ._main import main

import typing


class Bool:
    _p: int
    value: typing.Union[None, bool]
    _inversion: typing.Union[None, 'Bool']
    context: typing.Union[None, 'Context']

    __slots__ = '_p', 'value', '_inversion', 'context'

    def __init__(self) -> None:
        self.context = None

    @property
    def is_undef(self) -> bool:
        return self.context is None

    @property
    def id(self) -> int:
        assert self.context is not None
        return self.context._get_id(self._p)

    @property
    def kind(self) -> str:
        assert self.context is not None
        return self.context._get_kind(self._p)

    @property
    def is_false(self) -> bool:
        return self.kind == 'false'

    @property
    def is_true(self) -> bool:
        return self.kind == 'true'

    @property
    def is_const(self) -> bool:
        return self.kind in ('false', 'true')

    @property
    def is_term(self) -> bool:
        return self.kind == 'term'

    @property
    def term(self) -> typing.Hashable | None:
        assert self.is_term
        assert self.context is not None
        return self.context._get_term(self._p)

    @property
    def args(self) -> list['Bool']:
        assert self.kind not in ('false', 'true', 'not', 'term')
        assert self.context is not None
        return [self.context._make(a)
                for a in self.context._get_args(self._p)]

    def __repr__(self) -> str:
        v = self.kind
        if v == 'term':
            v = repr(self.term)
        return f'<{self.__class__.__name__} {v}>'

    def __str__(self) -> str:
        assert self.context is not None
        return self.context._print(self._p)

    def __invert__(self) -> 'Bool':
        assert self.context is not None
        return self.context.get_inversion(self)

    def __or__(self, other: 'Bool') -> 'Bool':
        assert self.context is not None
        return self.context.get_or(self, other)

    def __and__(self, other: 'Bool') -> 'Bool':
        assert self.context is not None
        return self.context.get_and(self, other)

    ''' TODO
    def __eq__(self, other: object) -> bool:
        assert isinstance(other, Bool)
        assert other.context is self.context
        return self.id == other.id
    '''


class Context(_Context):
    def __init__(self, bool_type: typing.Type[Bool] = Bool) -> None:
        self.__t = bool_type
        self.__terms: dict[typing.Hashable, Bool] = {}
        self.__nodes: dict[int, Bool] = {}

    def _make(self, p: int) -> Bool:
        b = self.__nodes.get(p)
        if b is None:
            b = self.__nodes[p] = self.__t()
            b._p = p
            b.value = None
            id = self._get_id(p)
            if id in (0, 1):
                b.value = bool(id)
            b._inversion = None
            b.context = self

        return b

    def get(self, t: typing.Hashable) -> Bool:
        b = self.__terms.get(t)
        if b is None:
            b = self.__terms[t] = self._make(self._get(t))

        return b

    @property
    def false(self) -> Bool:
        return self.get(False)

    @property
    def true(self) -> Bool:
        return self.get(True)

    def get_inversion(self, arg: Bool) -> Bool:
        if arg._inversion is None:
            arg._inversion = self._make(arg._p ^ 1)
            arg._inversion._inversion = arg
        return arg._inversion

    def get_or(self, *args: Bool) -> Bool:
        assert all(a.context is self for a in args)
        return self._make(self._get_or(*(a._p for a in args)))

    def get_and(self, *args: Bool) -> Bool:
        return self.get_inversion(self.get_or(*(self.get_inversion(a) for a in args)))

    def get_eq(self, a: Bool, b: Bool) -> Bool:
        assert all(a.context is self for a in (a, b))
        return self._make(self._get_eq(a._p, b._p))

    def ifelse(self, i: Bool, t: Bool, e: Bool) -> Bool:
        assert all(a.context is self for a in (i, t, e))
        return self._make(self._ifelse(i._p, t._p, e._p))

    def is_equiv(self, a: Bool, b: Bool) -> bool:
        assert all(a.context is self for a in (a, b))
        return self._is_equiv(a._p, b._p)
