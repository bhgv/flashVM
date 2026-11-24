# Copyright (c) 2026 bhgv
# EN: Exercise contact booleans through the public brush and SVG model.
# RU: Проверять контактные операции через публичную модель кисти и SVG.

implement IdepaintContacts;
include "sys.m";
include "idevector.m";

sys: Sys;     # Failure reporting through this test process.
v: Idevector; # Real model loaded by the hosted runtime.


# EN: This isolated application runs the four vector contact cases.
# RU: Изолированное приложение выполняет четыре сценария контакта контуров.
IdepaintContacts: module
{
    init: fn(); # Test application's VM entry.
};


# EN: Halt this test for an invalid predicate, naming the broken contract.
# RU: Остановить тест при неверном условии, сообщив нарушенный контракт.
require(ok: int, message: string)
{
    if (!ok) {
        sys->print("FAIL brush contact: %s\n", message);
        fd := sys->open("/dev/sysctl", Sys->OWRITE);
        if (fd != nil) {
            data := array of byte "halt";
            sys->write(fd, data, len data);
        }
        raise "fail: brush contact";
    }
}


# EN: Import one borrowed SVG path list at the ordinary page extent.
# RU: Импортировать список путей SVG в обычном размере страницы.
open(paths: string): ref Idevector->Document
{
    (status, doc) := v->parse("<svg width=\"1000\" height=\"600\">"+paths+"</svg>");
    require(status == v->Ok, "load contact fixture");
    doc.width = 20;
    return doc;
}


# EN: Apply a released straight stroke on the owned document.
# RU: Нанести прямой мазок на документ этого теста.
stroke(doc: ref Idevector->Document, left, right, y: int)
{
    require(v->defer(doc, 1, left, y) == v->Ok &&
            v->defer(doc, 3, right, y) == v->Ok && v->refine(doc) == v->Ok, "refine contact gesture");
}


# EN: Return how many borrowed nodes use the named verb.
# RU: Вернуть число команд заданного типа в заимствованном пути.
verbs(shape: ref Idevector->Shape, verb: int): int
{
    count := 0;
    for (i := 0; i < len shape.nodes; i++) {
        count += shape.nodes[i].verb == verb;
    }
    return count;
}


# EN: Look for an untouched cubic by the two handles' horizontal coordinates.
# RU: Найти нетронутую кубическую кривую по X двух ручек.
cubic(shape: ref Idevector->Shape, first, second: int): int
{
    for (i := 0; i < len shape.nodes; i++) {
        node := shape.nodes[i];
        if (node.verb == v->Cubic && node.cx == first && node.c2x == second) {
            return 1;
        }
    }
    return 0;
}


# EN: Ensure the exact published path and join text survive another SVG import.
# RU: Убедиться, что путь и стыки сохраняются после повторного импорта SVG.
roundtrip(doc: ref Idevector->Document)
{
    saved := v->svg(doc);
    (status, reopened) := v->parse(saved);
    require(status == v->Ok && v->svg(reopened) == saved, "SVG round trip");
}


# EN: The brush only touches the older blue fill along a partial straight seam.
# Both sides of the seam become filled, and the original upper cubic is retained.
# RU: Кисть встречает синюю заливку вдоль части прямой кромки. Обе стороны
# общей кромки закрашиваются. Верхняя кубическая кривая сохраняется.
straightseam()
{
    doc := open("<path fill=\"#245ac4\" d=\"M 100 100 C 145 65 260 65 300 100 L 300 170 L 100 170 Z\"/>");
    original := v->svg(doc);
    stroke(doc, 160, 380, 180);
    require(len doc.shapes == 1 && verbs(doc.shapes[0], v->Move) == 1 &&
            cubic(doc.shapes[0], 145, 260), "partly coincident straight seam unites both fills");
    roundtrip(doc);
    require(v->history(doc, 0) && v->svg(doc) == original, "one Undo restores overlapping seam sources");
}


# EN: Subtraction uses the same-side shared top edge as overlap. Unrelated
# red handles survive, and moving the blue stroke reveals a real cut.
# RU: При разности общая верхняя кромка имеет заливку с той же стороны.
# Ручки красной фигуры сохраняются. Перенос синего мазка открывает вырез.
subtraction()
{
    doc := open("<path fill=\"#e04444\" d=\"M 100 170 L 300 170 L 300 235 C 260 285 140 285 100 235 Z\"/>");
    original := v->svg(doc);
    stroke(doc, 70, 340, 180);
    require(len doc.shapes == 2 && cubic(doc.shapes[0], 260, 140), "shared top edge cuts without refitting red cubic");
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, 205, 180) == v->Ok && v->pointer(doc, 3, 205, 350) == v->Ok,
            "move stroke from shared edge");
    require(v->pointer(doc, 1, 205, 180) == v->Ok && doc.edit.selected == -1, "cut remains behind moved stroke");
    v->cancel(doc);
    require(v->history(doc, 0) && v->history(doc, 0) && v->svg(doc) == original, "Undo shared-edge subtraction");
}


# EN: Tangency at the interior of a quadratic and a straight nib side
# creates two source-degree fragments but leaves two separately picked figures.
# RU: Касание внутренней точки квадратичной кривой прямой кромкой кисти
# даёт два квадратичных участка и две отдельно выделяемые фигуры.
pointcontact()
{
    source := "<path fill=\"#245ac4\" d=\"M 100 100 C 140 70 260 70 300 100 L 300 160 Q 200 200 100 160 Z\"/>";
    doc := open(source);
    stroke(doc, 130, 270, 190);
    require(len doc.shapes == 2, "point-only contact does not join figures");
    upper := -1;
    for (i := 0; i < len doc.shapes; i++) {
        if (cubic(doc.shapes[i], 140, 260)) {
            upper = i;
        }
    }
    require(upper >= 0 && verbs(doc.shapes[upper], v->Quadratic) == 2,
            "interior tangency retains split quadratics and the original cubic");
    roundtrip(doc);
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, 200, 150) == v->Ok && doc.edit.selected == upper, "pick upper fill");
    v->cancel(doc);
    require(v->pointer(doc, 1, 200, 192) == v->Ok && doc.edit.selected >= 0 &&
            doc.edit.selected != upper, "pick tangent stroke separately");
    v->cancel(doc);
}


# EN: Two imported fills own opposite sides of one complete cubic edge.
# A stroke reaching them removes that seam and keeps the exterior handles.
# RU: Две импортированные заливки лежат с разных сторон одного кубического
# сегмента. Мазок удаляет общую границу и сохраняет внешние ручки.
curveseam()
{
    left := "<path fill=\"#245ac4\" d=\"M 100 100 C 128 62 191 62 210 100 " +
        "C 250 141 181 209 210 250 L 100 250 Z\"/>";
    right := "<path fill=\"#245ac4\" d=\"M 210 100 C 241 63 310 63 340 100 L 340 250 L 210 250 " +
        "C 181 209 250 141 210 100 Z\"/>";
    doc := open(left+right);
    stroke(doc, 55, 150, 193);
    require(len doc.shapes == 1 && verbs(doc.shapes[0], v->Move) == 1 &&
            cubic(doc.shapes[0], 128, 191) && cubic(doc.shapes[0], 241, 310) &&
            !cubic(doc.shapes[0], 250, 181) && !cubic(doc.shapes[0], 181, 250),
            "opposite identical cubic seam disappears and both outside cubics survive");
    roundtrip(doc);
}


# EN: Start the real model and run every supported contact case.
# RU: Запустить модель и проверить все поддерживаемые типы контакта.
init()
{
    sys = load Sys Sys->PATH;
    v = load Idevector Idevector->PATH;
    require(v != nil, "load vector model");
    straightseam();
    subtraction();
    pointcontact();
    curveseam();
    sys->print("PASS brush contacts: shared line, same-side subtraction, tangent Q and shared cubic\n");
}
