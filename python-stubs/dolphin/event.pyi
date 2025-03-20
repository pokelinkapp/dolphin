"""
Module for awaiting or registering callbacks on all events emitted by Dolphin.

The odd-looking Protocol classes are just a lot of syntax to essentially describe
the callback's signature. See https://www.python.org/dev/peps/pep-0544/#callback-protocols
"""
from typing import Protocol, type_check_only
from collections.abc import Callable


def on_frameadvance(callback: Callable[[], None] | None) -> None:
    """
    Registers a callback to be called every time the game has rendered a new frame.
    """


async def frameadvance() -> None:
    """
    Awaitable event that completes once the game has rendered a new frame.
    """


@type_check_only
class _MemorybreakpointCallback(Protocol):
    def __call__(self, is_write: bool, addr: int, value: int) -> None:
        """
        Example callback stub for on_memorybreakpoint.

        :param is_write: true if a value was written, false if it was read
        :param addr: address that was accessed
        :param value: new value at the given address
        """


def on_memorybreakpoint(callback: _MemorybreakpointCallback | None) -> None:
    """
    Registers a callback to be called every time a previously added memory breakpoint is hit.

    :param callback:
    :return:
    """


async def memorybreakpoint() -> tuple[bool, int, int]:
    """
    Awaitable event that completes once a previously added memory breakpoint is hit.
    """


@type_check_only
class _CodebreakpointCallback(Protocol):
    def __call__(self, addr: int) -> None:
        """
        Example callback stub for on_codebreakpoint.

        :param addr: address of the instruction being executed
        """


def on_codebreakpoint(callback: _CodebreakpointCallback | None) -> None:
    """
    Registers a callback to be called every time a previously added code breakpoint is hit.

    :param callback:
    :return:
    """


async def codebreakpoint() -> int:
    """
    Awaitable event that completes once a previously added code breakpoint is hit.
    """


@type_check_only
class _FramedrawnCallback(Protocol):
    def __call__(self, width: int, height: int, data: bytes) -> None:
        """
        Example callback stub for on_framedrawn.

        :param width: width of the drawn frame
        :param height: height of the drawn frame
        :param data: bytes representing RGB pixels, of length width*height
        """


def on_framedrawn(callback: _FramedrawnCallback | None) -> None:
    """
    Registers a callback to be called every time a frame is drawn.
    Note that this event may negatively impact performance a bit.

    :param callback:
    :return:
    """


async def framedrawn() -> tuple[int, int, bytes]:
    """
    Awaitable event that completes once a frame is drawn.
    Note that this event may negatively impact performance a bit.
    """
