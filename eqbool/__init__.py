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
    def __init__(self, v):
        self.__v = v


class Context(_Context):
    def get_false(self):
        return Bool(self._get_false())

