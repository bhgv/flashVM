# Copyright (c) 2026 bhgv
# EN: Exercise contact booleans through the public brush and SVG model.
# RU: Проверять контактные операции через публичную модель кисти и SVG.

implement IdepaintContacts;
include "sys.m";
include "idevector.m";

sys: Sys;     # Failure reporting through this test process.
v: Idevector; # Real model loaded by the hosted runtime.


# EN: This isolated application runs the vector contact cases.
# RU: Изолированное приложение проверяет контакты векторных контуров.
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


# EN: The right fill owns exactly the first half of the left fill's cubic,
# reversed. The second half remains an editable cubic after joining them.
# RU: Правая заливка совпадает с первой половиной левой кубики в обратном
# направлении. Вторая половина остаётся редактируемой кубикой.
partialseam()
{
    left := "<path fill=\"#245ac4\" d=\"M 100 100 C 128 62 191 62 210 100 " +
        "C 250 140 170 220 210 260 L 100 260 Z\"/>";
    right := "<path fill=\"#245ac4\" d=\"M 210 100 C 241 63 310 63 340 100 L 340 180 L 210 180 " +
        "C 220 150 230 120 210 100 Z\"/>";
    doc := open(left+right);
    before := v->svg(doc);
    stroke(doc, 55, 150, 225);
    require(len doc.shapes == 1 && verbs(doc.shapes[0], v->Move) == 1 &&
            cubic(doc.shapes[0], 128, 191) && cubic(doc.shapes[0], 241, 310) &&
            cubic(doc.shapes[0], 200, 190) && !cubic(doc.shapes[0], 220, 230),
            "partial reversed cubic seam removes only its shared half");
    roundtrip(doc);
    require(v->history(doc, 0) && v->svg(doc) == before, "Undo partial shared cubic");
}


# EN: Opposite quadratics touch at a sloping tangent, at 2/3 and 1/3 of
# their parameters. The brush joins only the lower lobe along its left edge.
# RU: Противоположные квадратичные кривые касаются под наклоном при параметрах
# 2/3 и 1/3. Кисть присоединяется только к левой кромке нижней фигуры.
twocurves()
{
    upper := "<path fill=\"#245ac4\" d=\"M 140 85 C 176 48 279 48 320 85 " +
        "L 320 140 Q 230 230 140 140 Z\"/>";
    lower := "<path fill=\"#245ac4\" d=\"M 140 180 Q 230 150 320 300 " +
        "L 320 370 C 278 394 172 394 140 370 Z\"/>";
    doc := open(upper+lower);
    stroke(doc, 90, 175, 250);
    require(len doc.shapes == 2 && verbs(doc.shapes[0], v->Quadratic) == 2 &&
            verbs(doc.shapes[1], v->Quadratic) == 2,
            "sloping interior Q/Q contact retains two split quadratic outlines");
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, 215, 135) == v->Ok && doc.edit.selected >= 0, "pick upper tangent fill");
    chosen := doc.edit.selected;
    v->cancel(doc);
    require(v->pointer(doc, 1, 215, 250) == v->Ok && doc.edit.selected >= 0 && doc.edit.selected != chosen,
            "pick the lower tangent fill separately");
    v->cancel(doc);
    roundtrip(doc);
}


# EN: Find a straight seam encoded with nonuniform cubic controls. The
# exposed section stays a cubic with its own original parameterization.
# RU: Найти прямую кромку, заданную неравномерными кубическими ручками.
# Оставшийся участок сохраняет свой исходный тип и параметризацию.
encodedseam()
{
    source := "<path fill=\"#245ac4\" d=\"M 100 100 C 145 65 260 65 300 100 L 300 170 " +
        "C 280 170 160 170 100 170 Z\"/>";
    doc := open(source);
    stroke(doc, 160, 380, 180);
    require(len doc.shapes == 1 && cubic(doc.shapes[0], 145, 260), "merge cubic-encoded straight seam");
    shape := doc.shapes[0];
    exposed := 0;
    for (i := 0; i < len shape.nodes; i++) {
        node := shape.nodes[i];
        exposed += node.verb == v->Cubic && node.x == 100 && node.y == 170 && node.cy == 170 &&
            node.c2y == 170 && node.cx < 160 && node.c2x > 100;
    }
    require(exposed == 1, "exposed collinear span retains its nonuniform cubic verb");
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
    partialseam();
    twocurves();
    encodedseam();
    sys->print("PASS Bezier contacts: partial cubic, angled Q/Q and cubic-encoded line\n");
}
