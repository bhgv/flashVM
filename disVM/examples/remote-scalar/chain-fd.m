# Copyright (c) 2026 bhgv
# EN: Common interface for both remote file checks in the chain.
# RU: Общий интерфейс двух удалённых вызовов для проверки файлов.
Chainfd: module {
    forward: fn(input, output: int): int;
};
