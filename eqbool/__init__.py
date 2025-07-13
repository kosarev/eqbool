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

    @classmethod
    def _make(cls, __c: 'Context', __v: _Bool) -> 'Bool':
        assert isinstance(__c, Context)
        assert type(__v) is _Bool
        b = cls()
        b.context = __c
        b._set(__v)
        return b

    @property
    def void(self) -> bool:
        return self.context is None

    @property
    def id(self) -> int:
        assert not self.void
        return self._get_id()

    def __str__(self) -> str:
        assert not self.void
        return self._print()

    def __invert__(self) -> 'Bool':
        assert self.context is not None
        return type(self)._make(self.context, self._invert())

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

    def _make(self, v: _Bool) -> Bool:
        return self.__t._make(self, v)

    def get(self, v: bool | str | int | typing.Tuple[typing.Any, ...]) -> Bool:
        return self._make(self._get(v))

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

    def ifelse(self, i: Bool, t: Bool, e: Bool) -> Bool:
        assert all(a.context is self for a in (i, t, e))
        return self._make(self._ifelse(i, t, e))
