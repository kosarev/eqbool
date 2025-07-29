
import typing

class _Bool:
    def _set(self, v: _Bool) -> None:
        ...

    def _get_id(self) -> int:
        ...

    def _get_kind(self) -> str:
        ...

    def _get_term(self) -> typing.Hashable:
        ...

    def _get_args(self) -> list[_Bool]:
        ...

    def _invert(self) -> _Bool:
        ...

    def _print(self) -> str:
        ...


class _Context:
    def _get(self, v: typing.Hashable) -> _Bool:
        ...

    def _get_or(self, *args: _Bool) -> _Bool:
        ...

    def _ifelse(self, i: _Bool, t: _Bool, e: _Bool) -> _Bool:
        ...

    def _get_eq(self, a: _Bool, b: _Bool) -> _Bool:
        ...
