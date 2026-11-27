# Copyright (c) 2026 bhgv
# EN: Replacement image with a different signature catalogue at the same path.
# RU: Образ замены с другим каталогом сигнатур по тому же пути.
implement Replacement;

Replacement: module {
    other: fn(input: int): int;
};


# EN: Return a distinguishable result for the replacement's only supported method.
# RU: Вернуть отличимый результат единственного поддерживаемого метода замены.
other(input: int): int
{
    return input - 7;
}
