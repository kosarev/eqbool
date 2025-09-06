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
    _value: typing.Union[None, bool]
    _inversion: typing.Union[None, 'Bool']
    context: typing.Union[None, 'Context']

    __slots__ = '_p', '_value', '_inversion', 'context'

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

    def __xor__(self, other: 'Bool') -> 'Bool':
        assert self.context is not None
        return self.context.get_neq(self, other)

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
        self.false, self.true = self.get(False), self.get(True)

    def _make(self, p: int) -> Bool:
        b = self.__nodes.get(p)
        if b is None:
            b = self.__nodes[p] = self.__t()
            b._p = p
            b._value = None
            id = self._get_id(p)
            if id in (0, 1):
                b._value = bool(id)
            b._inversion = None
            b.context = self

        return b

    def get(self, t: typing.Hashable) -> Bool:
        b = self.__terms.get(t)
        if b is None:
            b = self.__terms[t] = self._make(self._get(t))

        return b

    def get_inversion(self, arg: Bool) -> Bool:
        if arg._inversion is None:
            arg._inversion = self._make(arg._p ^ 1)
            arg._inversion._inversion = arg
        return arg._inversion

    def get_or(self, *args: Bool) -> Bool:
        assert all(a.context is self for a in args)

        # Calls are expensive in Python, so do the simplest reductions
        # right away.
        unique_args = []
        for a in args:
            if a._value is not None:
                if a._value is False:
                    continue
                return self.true
            if a._inversion in unique_args:
                return self.true
            if a not in unique_args:
                unique_args.append(a)

        if len(unique_args) == 0:
            return self.false
        if len(unique_args) == 1:
            return unique_args[0]

        return self._make(self._get_or(*(a._p for a in args)))

    def get_and(self, *args: Bool) -> Bool:
        assert all(a.context is self for a in args)

        # Calls are expensive in Python, so do the simplest reductions
        # right away.
        unique_args = []
        for a in args:
            if a._value is not None:
                if a._value is True:
                    continue
                return self.false
            if a._inversion in unique_args:
                return self.false
            if a not in unique_args:
                unique_args.append(a)

        if len(unique_args) == 0:
            return self.true
        if len(unique_args) == 1:
            return unique_args[0]

        return self._make(self._get_or(*(a._p ^ 1 for a in unique_args)) ^ 1)

    def get_eq(self, a: Bool, b: Bool) -> Bool:
        assert all(a.context is self for a in (a, b))
        return self._make(self._get_eq(a._p, b._p))

    def get_neq(self, a: Bool, b: Bool) -> Bool:
        return ~self.get_eq(a, b)

    def ifelse(self, i: Bool, t: Bool, e: Bool) -> Bool:
        assert all(a.context is self for a in (i, t, e))

        # Calls are expensive in Python, so do the simplest reductions
        # right away.
        if i._value is not None:
            return t if i._value else e
        if t._value is not None:
            return (i | e) if t._value else (~i & e)
        if e._value is not None:
            return (~i | t) if e._value else (i & t)

        if t is e:
            # i ? t : t
            return t
        if t is e._inversion:
            # TODO: Is this really a simplification?
            #       ifelse(cond, a, ~a) is just the same set of
            #       clauses as eq(cond, a).
            # cond ? a : ~a
            return self.get_eq(i, t)

        if i is t or i is e._inversion:
            #  a ? a : b
            # ~b ? a : b
            return t | e
        if i is e or i is t._inversion:
            #  b ? a : b
            # ~a ? a : b
            return t & e

        return self._make(self._ifelse(i._p, t._p, e._p))

    def is_equiv(self, a: Bool, b: Bool) -> bool:
        assert all(a.context is self for a in (a, b))
        return self._is_equiv(a._p, b._p)
