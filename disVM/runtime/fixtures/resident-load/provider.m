# Copyright (c) 2026 bhgv
# EN: Typed interface for independent module-data instances loaded by residents.
# RU: Типизированный интерфейс независимых экземпляров данных загружаемого модуля.
Burst: module {
    peek: fn(): int; # Read this instance scalar through a self link.
    run: fn(value: int): int; # Retain private module state across a native yield.
};
