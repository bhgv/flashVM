# Собственные индексы результатов / Own Array result slots — v0.129

ActionInitArray и String.split создают собственные индексные данные результата,
обходя унаследованные accessor, readonly и обычные assignment watcher. Общий
внутренний помощник копирует строковые данные в контекст и проверяет принадлежность
результата/значения. Он не меняет ранее захваченную длину. String.split записывает
итоговую длину непосредственно после формирования частей. Slice/splice/concat
используют тот же помощник вместо прежней локальной функции без изменения
их порядка чтения исходного массива.

avm1_array_literal_own устанавливает getter/setter и readonly accessor в
Array.prototype, проверяет обе собственные строки и длину 2 у литерала и split,
нулевое число вызовов accessor и сохранность литерала после GC. Правило
InitArray сверено с ArrayBuilder::init_with pinned Ruffle (define_value).
Результаты продолжают использовать сохранённый встроенный прототип.

Это не полная семантика всех методов Array. Собственные дескрипторы, флаги длины,
версии функций и конструкторов и все стадии роудмапа остаются открытыми.

## English

ActionInitArray and String.split define own indexed data without inherited
accessors, readonly checks or assignment watchers. A shared private helper
copies string storage into the context and validates ownership. It preserves
captured result length; split records the final count directly. Slice/splice/
concat reuse the same helper without changing source-read ordering.

avm1_array_literal_own installs inherited getter/setter and readonly accessors,
checks both own elements and length, verifies no callback invocation and collects
before rereading the rooted literal. InitArray follows the pinned ArrayBuilder
own define_value policy. Retained builtin result prototypes remain in use.
Full Array descriptors/length flags, constructor/function version provenance
and every roadmap stage remain open.
