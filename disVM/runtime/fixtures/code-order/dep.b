# Copyright (c) 2026 bhgv
# EN: Test driver substitutes one SOURCE marker for each dependency image.
# RU: Test driver заменяет SOURCE marker для каждой dependency image.
implement Dep;
include "dep.m";


# EN/RU: Return the compiled dependency marker; no resource lookup.
# Вернуть marker скомпилированной dependency без resource lookup.
value(): string
{
    return "SOURCE";
}
