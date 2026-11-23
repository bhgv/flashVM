# Copyright (c) 2026 bhgv
# EN: Dis export consumed from a real FlashVM AVM1 context.
# RU: Экспорт Dis для вызова из настоящего контекста AVM1.
implement Limbo;

Limbo: module {
    next: fn(n: int): int;
};


# EN: Return the incremented value or demonstrate a remote error on zero.
# RU: Вернуть увеличенное значение или показать удалённую ошибку для нуля.
next(n: int): int
{
    if (n == 0) {
        raise "zero rejected by Dis";
    }
    return n + 1;
}
