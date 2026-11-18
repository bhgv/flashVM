# Copyright (c) 2026 bhgv
# EN: Ordinary typed dependency with a test-selected value, compiled under one stable source/output name.
# RU: Обычная typed-зависимость с выбранным тестом значением, компилируемая под одним source/output именем.
implement Value;
include "value.m";


# EN/RU: Return the immutable compiled value; no host, namespace or transport callbacks.
# Вернуть immutable compiled value, без host/namespace/transport callbacks.
value(): int
{
    return EXPECTED_VALUE;
}
