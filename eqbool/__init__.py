# -*- coding: utf-8 -*-

#   Testing boolean expressions for equivalence.
#   https://github.com/kosarev/eqbool
#
#   Copyright (C) 2023-2025 Ivan Kosarev.
#   mail@ivankosarev.com
#
#   Published under the MIT license.


from ._eqbool import _Bool
from ._eqbool import _Context
from ._main import main

import typing


class Bool(_Bool):
    def __init__(self) -> None:
        self.context: None | Context = None

    @property
    def is_undef(self) -> bool:
        return self.context is None

    @property
    def id(self) -> int:
        assert not self.is_undef
        return self._get_id()

    def __str__(self) -> str:
        assert not self.is_undef
        return self._print()

    def __invert__(self) -> 'Bool':
        assert self.context is not None
        return self.context._make(self._invert())

    def __or__(self, other: 'Bool') -> 'Bool':
        assert self.context is not None
        return self.context.get_or(self, other)

    def __and__(self, other: 'Bool') -> 'Bool':
        assert self.context is not None
        return self.context.get_and(self, other)

    def __eq__(self, other: object) -> bool:
        assert isinstance(other, Bool)
        assert other.context is self.context
        return self.id == other.id


class Context(_Context):
    def __init__(self, bool_type: typing.Type[Bool] = Bool) -> None:
        self.__t = bool_type
        self.__terms: dict[typing.Hashable, Bool] = {}

    def _make(self, v: _Bool) -> Bool:
        assert type(v) is _Bool
        b = self.__t()
        b.context = self
        b._set(v)
        return b

    def get(self, t: typing.Hashable) -> Bool:
        b = self.__terms.get(t)
        if b is None:
            b = self._make(self._get(t))
            self.__terms[t] = b

        return b

    @property
    def false(self) -> Bool:
        return self.get(False)

    @property
    def true(self) -> Bool:
        return self.get(True)

    def get_or(self, *args: Bool) -> Bool:
        assert all(a.context is self for a in args)
        return self._make(self._get_or(*args))

    def get_and(self, *args: Bool) -> Bool:
        return ~self.get_or(*(~a for a in args))

    def get_eq(self, a: Bool, b: Bool) -> Bool:
        assert all(a.context is self for a in (a, b))
        return self._make(self._get_eq(a, b))

    def ifelse(self, i: Bool, t: Bool, e: Bool) -> Bool:
        assert all(a.context is self for a in (i, t, e))
        return self._make(self._ifelse(i, t, e))
