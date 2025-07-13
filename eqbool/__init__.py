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


class Bool(_Bool):
    def __init__(self):
        self.context = None

    @classmethod
    def _make(cls, __c, __v):
        assert isinstance(__c, Context)
        assert type(__v) is _Bool
        b = cls()
        b.context = __c
        b._set(__v)
        return b

    @property
    def void(self):
        return self.context is None

    @property
    def id(self):
        assert not self.void
        return self._get_id()

    def __str__(self):
        assert not self.void
        return self._print()

    def __invert__(self):
        return type(self)._make(self.context, self._invert())

    def __or__(self, other):
        return self.context.get_or(self, other)

    def __eq__(self, other):
        assert other.context is self.context
        return self.id == other.id


class Context(_Context):
    def __init__(self, bool_type=Bool):
        self.__t = bool_type

    def _make(self, v):
        return self.__t._make(self, v)

    def get(self, v):
        return self._make(self._get(v))

    @property
    def false(self):
        return self.get(False)

    @property
    def true(self):
        return self.get(True)

    def get_or(self, *args):
        assert all(a.context is self for a in args)
        return self._make(self._get_or(*args))

    def ifelse(self, i, t, e):
        assert all(a.context is self for a in (i, t, e))
        return self._make(self._ifelse(i, t, e))
