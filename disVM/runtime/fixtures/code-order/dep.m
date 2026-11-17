# Copyright (c) 2026 bhgv
# EN: Minimal transitive dependency interface for purpose-aware load checks.
# RU: Минимальный интерфейс transitive dependency для purpose-aware load checks.
Dep: module
{
    # EN/RU: Return the compiled fixture source marker, without side effects.
    # Вернуть marker скомпилированного fixture source без side effects.
    value: fn(): string;
};
