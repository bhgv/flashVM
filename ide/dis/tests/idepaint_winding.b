# Copyright (c) 2026 bhgv
# EN: Check nonzero winding in the public SVG brush and its Undo history.
# RU: Проверять ненулевой обход через публичную SVG-кисть и Undo.

implement IdepaintWinding;
include "sys.m";
include "idevector.m";

sys: Sys;     # Test reporting and requested halt.
v: Idevector; # Actual hosted document model.


# EN: Isolated native Dis test entry.
# RU: Точка входа изолированного нативного Dis-теста.
IdepaintWinding: module
{
    init: fn(); # Run each authored SVG fixture.
};


# EN: Halt this test on a failed predicate and name the rejected contract.
# RU: Остановить тест при нарушении условия и сообщить его имя.
require(ok: int, what: string)
{
    if (!ok) {
        sys->print("FAIL brush winding: %s\n", what);
        fd := sys->open("/dev/sysctl", Sys->OWRITE);
        if (fd != nil) {
            data := array of byte "halt";
            sys->write(fd, data, len data);
        }
        raise "fail: brush winding";
    }
}


# EN: Open a borrowed red path on the test page with a blue brush.
# RU: Открыть красную фигуру на тестовом листе с синей кистью.
openpath(path: string): ref Idevector->Document
{
    source := "<svg width=\"1000\" height=\"600\"><path fill=\"#e04444\" d=\""+path+"\"/></svg>";
    (status, doc) := v->parse(source);
    require(status == v->Ok && len doc.shapes == 1, "load crossed source");
    doc.width = 20;
    return doc;
}


# EN: Commit a borrowed sequence of two page positions as a brush gesture.
# RU: Нанести кистью штрих между двумя переданными точками листа.
stroke(doc: ref Idevector->Document, x0, x1, y: int)
{
    require(v->defer(doc, 1, x0, y) == v->Ok &&
            v->defer(doc, 3, x1, y) == v->Ok && v->refine(doc) == v->Ok, "brush gesture");
}


# EN: Pick a point exactly, rejecting stroke proximity, then cancel the drag.
# RU: Выделить фигуру точно в точке без допуска на близость и отменить перенос.
atpoint(doc: ref Idevector->Document, x, y: int): int
{
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, x, y) == v->Ok, "pick gesture");
    found := doc.edit.selected;
    v->cancel(doc);
    return found;
}


# EN: Move the upper blue result so old paint can be checked independently.
# RU: Перенести синюю фигуру, чтобы отдельно проверить прежнюю заливку.
movebrush(doc: ref Idevector->Document, x, y: int)
{
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, x, y) == v->Ok && doc.edit.selected == len doc.shapes-1 &&
            v->pointer(doc, 3, x, 430) == v->Ok, "move blue brush");
}


# EN: Match an exterior cubic by both original control locations. An
# unchanged geometric span may be reversed to put fill on its right.
# RU: Найти внешнюю кубику по обеим исходным ручкам. Неизменённый участок
# может быть обращён, чтобы заливка оставалась справа.
handles(shape: ref Idevector->Shape, x, y, x2, y2: int): int
{
    for (i := 0; i < len shape.nodes; i++) {
        n := shape.nodes[i];
        if (n.verb == v->Cubic &&
            ((n.cx == x && n.cy == y && n.c2x == x2 && n.c2y == y2) ||
             (n.cx == x2 && n.cy == y2 && n.c2x == x && n.c2y == y))) {
            return 1;
        }
    }
    return 0;
}


# EN: Compare all published commands after an independent SVG import.
# RU: Сравнить опубликованные команды после повторного импорта SVG.
roundtrip(doc: ref Idevector->Document)
{
    saved := v->svg(doc);
    (status, copy) := v->parse(saved);
    require(status == v->Ok && v->svg(copy) == saved, "winding SVG round trip");
}


# EN: A crossed single ring has two lobes meeting at a point. The cut
# creates a third red piece while retaining both source cubic spans.
# RU: Пересечённое кольцо даёт две доли с общей точкой. Разрез добавляет
# третью красную часть, сохраняя обе исходные кубики.
bowtie()
{
    doc := openpath("M 100 100 C 128 65 270 65 300 100 L 100 300 " +
        "C 128 335 270 335 300 300 L 100 100 Z");
    source := doc.shapes[0];
    stroke(doc, 70, 330, 140);
    require(len doc.shapes == 4, "bowtie cut gives three red pieces and the stroke");
    top := atpoint(doc, 200, 105);
    middle := atpoint(doc, 200, 174);
    bottom := atpoint(doc, 200, 230);
    require(top >= 0 && middle >= 0 && bottom >= 0 && top != middle && middle != bottom &&
            handles(doc.shapes[top], 128, 65, 270, 65) &&
            handles(doc.shapes[bottom], 128, 335, 270, 335), "both original bowtie cubics survive");
    roundtrip(doc);
    movebrush(doc, 180, 140);
    require(atpoint(doc, 180, 140) == -1, "bowtie cut remains empty when exposed");
    require(v->history(doc, 0) && v->history(doc, 0) && doc.shapes[0] == source,
            "Undo restores the exact self-crossing source");
}


# EN: A dot crosses the internal 1-to-2 winding boundary of overlapping
# rings. Both halves of the dot must cut, including the hidden internal seam.
# RU: Отпечаток пересекает внутренний переход обхода от 1 к 2. Вырезаться
# должны обе его половины, включая скрытую внутреннюю границу.
doublewinding()
{
    doc := openpath("M 100 90 C 145 60 250 60 300 90 L 300 300 L 100 300 Z " +
        "M 200 120 L 400 120 L 400 270 L 200 270 Z");
    old := v->svg(doc);
    stroke(doc, 300, 300, 180);
    require(len doc.shapes == 2 && handles(doc.shapes[0], 145, 60, 250, 60),
            "crossed positive rings preserve the outside cubic");
    roundtrip(doc);
    movebrush(doc, 300, 180);
    require(atpoint(doc, 295, 180) == -1 && atpoint(doc, 305, 180) == -1 &&
            atpoint(doc, 280, 180) == 0 && atpoint(doc, 320, 180) == 0,
            "whole vector hole crosses both winding counts");
    require(v->history(doc, 0) && v->history(doc, 0) && v->svg(doc) == old, "Undo overlapping positive rings");
}


# EN: Reversed overlapping subpaths cancel to zero in the intersection.
# The remaining negative lobe is filled under the nonzero rule.
# RU: Встречные подпути взаимно обнуляют обход в пересечении. Область
# с отрицательным обходом остаётся залитой по ненулевому правилу.
oppositewinding()
{
    doc := openpath("M 100 90 C 145 60 250 60 300 90 L 300 260 L 100 260 Z " +
        "M 220 140 L 220 320 L 390 320 L 390 140 Z");
    stroke(doc, 62, 350, 111);
    require(len doc.shapes == 4 && handles(doc.shapes[0], 145, 60, 250, 60),
            "opposite windings retain both filled sides and original cubic");
    left := atpoint(doc, 150, 180);
    right := atpoint(doc, 350, 190);
    require(left >= 0 && right >= 0 && left != right && atpoint(doc, 255, 180) == -1,
            "cancelled overlap stays empty with separate filled lobes");
    roundtrip(doc);
}


# EN: Fully repeated geometry has winding magnitude two. Retain one editable
# outside, and cut one hole without inserting duplicate arcs.
# RU: Полностью повторённая фигура имеет модуль обхода два. Оставить одну
# редактируемую наружную границу и отверстие без дублирования дуг.
retraced()
{
    path := "M 100 90 C 145 60 250 60 300 90 L 300 280 L 100 280 Z";
    doc := openpath(path+" "+path);
    stroke(doc, 200, 200, 165);
    require(len doc.shapes == 2 && handles(doc.shapes[0], 145, 60, 250, 60),
            "coincident original spans reduce to one vector boundary");
    before := 0;
    for (i := 0; i < len doc.shapes[0].nodes; i++) {
        before += doc.shapes[0].nodes[i].verb == v->Move;
    }
    require(before == 2, "repeated source leaves one outside and one hole");
    roundtrip(doc);
    movebrush(doc, 200, 165);
    require(atpoint(doc, 200, 165) == -1, "hole cuts doubled winding");
}


# EN: One cubic visits (264,168) twice. A later cut retains the explicit
# cubic loop and the unrelated lower cubic with its original controls.
# RU: Одна кубика дважды проходит через (264,168). Разрез сохраняет её
# петлю и независимую нижнюю кубику с исходными ручками.
singlecubic()
{
    doc := openpath("M 300 240 C 284 112 76 112 444 240 L 444 340 " +
        "C 412 370 330 370 300 340 Z");
    source := doc.shapes[0];
    stroke(doc, 270, 475, 285);
    require(len doc.shapes == 4, "one loop, two cut pieces, and the brush");
    eye := atpoint(doc, 244, 154);
    base := atpoint(doc, 360, 315);
    require(eye >= 0 && base >= 0 && eye != base && handles(doc.shapes[base], 412, 370, 330, 370),
            "single cubic loop and untouched lower handles");
    cubiccount := 0;
    for (i := 0; i < len doc.shapes[eye].nodes; i++) {
        cubiccount += doc.shapes[eye].nodes[i].verb == v->Cubic;
    }
    require(cubiccount >= 2, "loop remains an editable cubic contour");
    roundtrip(doc);
    require(v->history(doc, 0) && doc.shapes[0] == source, "Undo restores single-loop command sequence");
}


# EN: Run the crossed, opposite and coincident winding cases through the VM.
# RU: Проверить пересекающиеся, встречные и совпадающие обходы на VM.
init()
{
    sys = load Sys Sys->PATH;
    v = load Idevector Idevector->PATH;
    require(v != nil, "load SVG editor");
    bowtie();
    sys->print("PASS winding bowtie and preserved cubics\n");
    doublewinding();
    sys->print("PASS winding interior hole across 1 and 2\n");
    oppositewinding();
    sys->print("PASS winding cancellation and negative filled lobe\n");
    retraced();
    sys->print("PASS winding coincident spans and hole\n");
    singlecubic();
    sys->print("PASS winding loop within one cubic\n");
}
