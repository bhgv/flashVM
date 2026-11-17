# Copyright (c) 2026 bhgv
# EN: Test driver substitutes the SOURCE string for each provider/terminal image.
# RU: Test driver заменяет SOURCE string для provider/terminal image.
implement Dup;
include "dup.m";
include "dep.m";


# EN/RU: Return the compiled source marker; no resource lookup.
# Вернуть marker скомпилированного source без resource lookup.
tag(): string
{
    return "SOURCE";
}


# EN/RU: Exercise ordinary transitive load after parent-module admission.
# Проверить обычный transitive load после допуска parent-module.
dependent(): string
{
    dep := load Dep "/code/dep.dis";
    if (dep == nil) {
        raise "dependency missing";
    }
    return dep->value();
}
