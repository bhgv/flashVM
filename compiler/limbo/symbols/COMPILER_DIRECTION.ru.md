# Направление полного компилятора Limbo

Принятые требования, а не завершённые возможности:

1. Полный компилятор Limbo реализуется на читаемом C99 и на Limbo. Семантика модулей/типов/каналов/exceptions Dis и debug information сохраняет совместимость с активно развиваемой experiment.
2. Для обоих scanner/parser авторитетны Coco/R .atg. Используется готовый bhgv/coco-r-limbo, а не новый порт генератора. Token/production coverage и диагностика/source-span cases эквивалентны; embedded actions, attributes и predicates пишутся на целевом C/Limbo и требуют соответствующих реализаций.
3. Прежний широкий C frontend только распознаёт syntax, FIR value() adapter — численный subset. Это не полная замена настоящего компилятора. Следующим semantic stages нужны owned AST/source spans, scope/import/type/signature checks, typed lowering и Dis backend с .sbl emission.
4. Сохраняется и адаптируется имеющаяся отладка Dis. Для locals/types/functions нужно расширить чтение символов дальше данного source-map prefix. Compiler source positions, logical Dis PCs и native JIT maps остаются связанными.
5. Последующая AS-реализация также использует AS-порт Coco/R и .atg. AS-to-Dis/module interoperability — отдельная runtime-работа; один generated parser не является компилятором или рабочим module bridge.
6. На implementation/import checkpoints сверять upstream fixes. При импорте Dis изолируется в flashVM/external/dis/, прежний x86 сохраняется, новые x86-64/arm64/riscv64 JIT/debug принимаются отдельно. Зависимость IDE от Tk/Draw/crypto не вводится.
