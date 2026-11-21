# Copyright (c) 2026 bhgv
# EN: Same export name as Blend with a different compiled signature.
# RU: То же имя экспорта, что у Blend, с другой скомпилированной сигнатурой.
implement ScalarBlend;
ScalarBlend: module { blend: fn(value: int): int; };


# EN: Return a distinct value to prove which implementation was linked.
# RU: Вернуть отличимый результат для проверки связанной реализации.
blend(value: int): int
{
    return value + 2;
}
