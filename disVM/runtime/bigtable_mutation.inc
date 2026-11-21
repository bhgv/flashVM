/* Copyright (c) 2026 bhgv
 * EN: Atomic mutation entries for the dense and keyed parts of one hybrid table.
 * RU: Атомарные операции над плотной и ключевой частями одной гибридной таблицы.
 */

/* EN: Insert value at position in the borrowed table's dense prefix.
 * RU: Вставить значение по позиции в плотной части заимствованной таблицы. */
void Bigtable_insert(void *frame)
{
    F_Bigtable_insert *call = frame;
    FvmTableRequest request = {.operation = FVM_TABLE_INSERT, .first = call->position, .value = call->value};
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* EN: Append a value under the guard that determines the current dense length.
 * RU: Добавить значение под guard, который определяет текущую плотную длину. */
void Bigtable_push(void *frame)
{
    F_Bigtable_push *call = frame;
    FvmTableRequest request = {.operation = FVM_TABLE_PUSH, .value = call->value};
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* EN: Return status and the consumed last positional value on success.
 * RU: Вернуть статус и при успехе последнее извлечённое позиционное значение. */
void Bigtable_pop(void *frame)
{
    F_Bigtable_pop *call = frame;
    FvmTableRequest request = {.operation = FVM_TABLE_POP};
    table_apply(call->tab, &request);
    call->ret->t0 = request.status;
    call->ret->t1 = request.status == FHT_OK ? request.value : 0;
}


/* EN: Add an absent numeric key under the native table guard.
 * RU: Добавить отсутствующий числовой ключ под guard таблицы. */
void Bigtable_addindex(void *frame)
{
    F_Bigtable_addindex *call = frame;
    FvmTableRequest request = {.operation = FVM_TABLE_ADD, .key = numeric_key(call->key), .value = call->value};
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* EN: Add an absent byte-counted text key under the native table guard.
 * RU: Добавить отсутствующий строковый ключ с явной длиной под guard таблицы. */
void Bigtable_addtext(void *frame)
{
    F_Bigtable_addtext *call = frame;
    FvmTableRequest request = {.operation = FVM_TABLE_ADD, .key = string_key(call->key), .value = call->value};
    table_apply(call->tab, &request);
    *call->ret = request.status;
}
