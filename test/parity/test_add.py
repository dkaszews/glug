# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
def add(a: int, b: int) -> int:
    return a + b


def test_add() -> None:
    assert add(2, 3) == 5
