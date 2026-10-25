// Syntax-only example: as*_parse validates this file; it does not emit SWO/ABC.
// Создаём объект с одним свойством, чтобы показать проверку имени через in.
var object = {answer : 42};

// Membership is an expression here; no for-in separator needs to be selected.
trace("answer" in object);

// This in separates a for-in header; its right operand is the enumerated object.
for (var key in object)
{
    trace(key); // Syntax demonstration only; this frontend does not execute trace.
}

// Parentheses restore ordinary expressions inside a classic for initializer.
for (var present = ("answer" in object); present; present = false)
{
    trace(present); // The assignment stops the illustrative loop after one iteration.
}
