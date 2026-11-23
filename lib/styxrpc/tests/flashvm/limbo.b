# Copyright (c) 2026 bhgv
# EN: Dis export consumed from a real FlashVM AVM1 context.
# RU: Экспорт Dis для вызова из настоящего контекста AVM1.
implement Limbo;

Limbo: module {
    next: fn(n: int): int;
    echo: fn(text: string, data: array of byte): (string, array of byte);
    number: fn(value: real, label: string): (real, string);
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


# EN: Return a checked copy of the text and change one byte of the decoded array.
# RU: Вернуть проверенную копию строки и изменить байт декодированного массива.
echo(text: string, data: array of byte): (string, array of byte)
{
    if (text == nil && data == nil) {
        return (text, data);
    }
    if (text == "reject" || len data != 600 || data[0] != byte 42 || data[599] != byte 87) {
        raise "managed argument rejected";
    }
    data[1] = byte 99;
    return (text + "!", data);
}


# EN: Return the real bit pattern with its managed string argument.
# RU: Вернуть представление real вместе со строковым аргументом.
number(value: real, label: string): (real, string)
{
    return (value, label);
}
