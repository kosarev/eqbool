
import typing

class _Context:
    def _get_id(self, v: int) -> int:
        ...

    def _get_fp(self, v: int) -> int:
        ...

    def _get_kind(self, v: int) -> str:
        ...

    def _get_term(self, v: int) -> typing.Hashable:
        ...

    def _get_args(self, v: int) -> list[int]:
        ...

    def _print(self, v: int) -> str:
        ...

    def _get(self, v: typing.Hashable) -> int:
        ...

    def _get_or(self, *args: int) -> int:
        ...

    def _ifelse(self, i: int, t: int, e: int) -> int:
        ...

    def _get_eq(self, a: int, b: int) -> int:
        ...

    def _is_equiv(self, a: int, b: int) -> bool:
        ...


class _EquivSession:
    def __init__(self, context: _Context) -> None:
        ...

    def _is_equiv(self, a: int, b: int) -> bool:
        ...


class _OrderContext:
    def __init__(self, context: _Context) -> None:
        ...

    def _register_order(self, term: int, before: typing.Hashable,
                        after: typing.Hashable) -> None:
        ...

    def _is_never(self, e: int) -> bool:
        ...
