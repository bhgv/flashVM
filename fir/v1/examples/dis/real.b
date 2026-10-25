# A homogeneous real tutorial for our Coco/R → FIR → Dis compiler pipeline.
# Однородный вещественный пример для нашего Coco/R → FIR → Dis.
implement RealAnswer;


RealAnswer: module
{
    value: fn(): real; # The definition must repeat the interface's real result type.
};


# No parameters or heap owners; return the binary64 result captured by the numeric harness.
# Locals remain real, including inferred bindings. Decimal real literals avoid implicit int casts.
# Нет параметров и heap-владельцев; результат binary64 захватывается числовым harness.
# Локальные значения — real; десятичные real-литералы исключают неявные int-преобразования.
value(): real
{
    initial: real = 125.0; # Explicit real declaration / Явное real-объявление.
    {
        temporary := initial / 2.0; # Inferred real; divide preserves the fractional part 62.5.
        initial = temporary; # Update the outer binding; this survives leaving the lexical block.
    }
    return initial - 20.0; # Capture exactly 42.5 in cell zero; executed flag is word one.
}
