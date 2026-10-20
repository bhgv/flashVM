# Рабочий числовой source-профиль 0.4.0

Это реально понижаемый в FIR профиль, отдельно от широких синтаксических фронтендов.
[Грамматика](../compiler/src1.atg), [C99 lowering](../compiler/lower.c).

| Syntax | AS1/AS2/AS3 | Limbo value() |
| --- | --- | --- |
| `+`, `-`, `*`, groups, unary `+/-` | f64 | i32 wrap |
| `.5`, `2.`, `1e2`, `0x2a` | yes | rejected |
| `2r101010`, `16r2a` | rejected | radix 2..36, checked digits/range |
| `var a=1,b=a+1;` | yes | rejected |
| `const x:Number=1;` | AS3 only, immutable | rejected |
| `x: int=1;`, `x:=1;` | rejected | yes |
| `x+=1; x-=1; x*=2;` | yes | yes |
| `# comment` | rejected | yes |
| `;` | empty statement | empty statement |

Приоритет: unary, затем *, затем +/-. Бинарные операции левоассоциативны.
Знак перед литералом допускает i32 minimum; перед группой/переменной минус
понижается как умножение на -1, сохраняя знак нуля AS. `++`/`--` не трактуются
как два знака и отвергаются. Limbo-профиль остаётся i32: big/real не реализованы.
Значения вне диапазона, неверная цифра radix, присваивание const и объявление
const в AS1/AS2 — ошибки с сохранением существующего выходного файла.
Объявления обрабатываются слева направо, без hoisting и без неявных globals.

[AS2 пример](../examples/as2/syntax.as), [AS3 const](../examples/as3/syntax.as),
[Limbo пример](../examples/limbo/syntax.b). Соответствующие syntax.txt содержат
ожидаемые trace. Используйте fir1_source, fir1_opt, fir1 object и swl link,
как в [полном цикле](PIPELINE.ru.md). Потоки и управление через C API сохранены.
У AS2/AS3 доступна только Number-аннотация; нет строк, объектов, if/циклов,
функций AS, каналов Limbo и полного модульного ABI. // и /* */ в Limbo пока
сохранены как совместимость со старыми учебными примерами; родной комментарий #.
Пределы: 64 KiB, 256 имён, 1000 SSA-значений, 64 уровня групп/unary, 255 байтов токена.
