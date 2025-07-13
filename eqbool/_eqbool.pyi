
import typing

class _Bool:
    def _set(self, v: _Bool) -> None:
        ...

    def _get_id(self) -> int:
        ...

    def _invert(self) -> _Bool:
        ...

    def _print(self) -> str:
        ...


class _Context:
    def _get(self, v: str | int | typing.Tuple[typing.Any, ...]) -> _Bool:
        ...

    def _get_or(self, *args: _Bool) -> _Bool:
        ...

    def _ifelse(self, i: _Bool, t: _Bool, e: _Bool) -> _Bool:
        ...
