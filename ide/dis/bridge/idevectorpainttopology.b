# Copyright (c) 2026 bhgv
# EN: Preserve untouched curves in interior punches and split disconnected fills.
# RU: Сохранять нетронутые кривые при внутренних вырезах и разделять островки заливки.


# EN: One closed contour borrowed from the completed paint result.
# RU: Один замкнутый обход, заимствованный из результата рисования.
PaintLoop: adt
{
    nodes: array of Node; # Immutable slice including Move and Close.
    modes: array of int;  # Optional matching join-mode slice.
    area: big; # Signed doubled area for winding and hole nesting.
    owner: int; # Outer loop index or -1 until the hole is assigned.
};


# EN: Return the signed winding contribution of edge a-b for a ray from p.
# RU: Вернуть знаковый вклад ребра a-b в обход по лучу из точки p.
paintcrossing(a, b, p: Point): int
{
    cross := (b.x-a.x)*(p.y-a.y) - (b.y-a.y)*(p.x-a.x);
    if (a.y <= p.y && p.y < b.y && cross > 0) {
        return 1;
    }
    if (b.y <= p.y && p.y < a.y && cross < 0) {
        return -1;
    }
    return 0;
}


# EN: Observe winding at p and signed area from a borrowed path. Sample curves
# at the same resolution as the coverage path. No input geometry is modified.
# RU: Измерить обход в p и знаковую площадь заимствованного пути. Разбиение
# кривых совпадает с расчётом покрытия. Исходная геометрия не меняется.
paintmeasure(nodes: array of Node, p: Point, work: ref Work): (big, int)
{
    area := big 0;
    winding := 0;
    start := Point(0, 0);
    previous := start;
    open := 0;
    for (i := 0; i < len nodes; i++) {
        if (i % 128 == 0) {
            checkwork(work);
        }
        node := nodes[i];
        finish := Point(node.x, node.y);
        if (node.verb == Move || node.verb == Close) {
            if (open) {
                area += big (previous.x*start.y - previous.y*start.x);
                winding += paintcrossing(previous, start, p);
            }
            start = finish;
            previous = finish;
            open = node.verb == Move;
            continue;
        }
        steps := 1;
        if (node.verb == Quadratic || node.verb == Cubic) {
            steps = 32;
        }
        origin := previous;
        for (s := 1; s <= steps; s++) {
            if (steps > 1) {
                finish = curvepoint(origin, node, s, steps);
            }
            area += big (previous.x*finish.y - previous.y*finish.x);
            winding += paintcrossing(previous, finish, p);
            previous = finish;
        }
    }
    if (open) {
        area += big (previous.x*start.y - previous.y*start.x);
        winding += paintcrossing(previous, start, p);
    }
    return (area, winding);
}


# EN: Bound one original command from start to finish, including controls.
# RU: Ограничить исходную команду от начала до конца вместе с ручками.
paintnodebox(node: Node, start, finish: CutPoint): CutBox
{
    first := start;
    second := finish;
    if (node.verb == Cubic || node.verb == Quadratic) {
        first = cutpoint(node.cx, node.cy);
        second = first;
    }
    if (node.verb == Cubic) {
        second = cutpoint(node.c2x, node.c2y);
    }
    return cutbounds(CutCurve(node.verb, start, first, second, finish));
}


# EN: Bound the brush and prove no source edge reaches its control box.
# The source winding must be constant there, even if a redundant subpath
# changes it elsewhere inside the same visibly filled shape.
# RU: Ограничить кисть и убедиться, что ни одно исходное ребро не достигает
# её оболочки. Обход должен быть постоянным, даже если лишний подпуть
# меняет его в пределах одной видимой заливки.
paintpunchclear(nodes, brush: array of Node, work: ref Work): int
{
    origin := cutpoint(brush[0].x, brush[0].y);
    box := CutBox(origin.x, origin.y, origin.x, origin.y);
    previous := origin;
    for (i := 1; i < len brush; i++) {
        node := brush[i];
        if (node.verb == Close) {
            break;
        }
        finish := cutpoint(node.x, node.y);
        bound := paintnodebox(node, previous, finish);
        if (bound.left < box.left) {
            box.left = bound.left;
        }
        if (bound.right > box.right) {
            box.right = bound.right;
        }
        if (bound.top < box.top) {
            box.top = bound.top;
        }
        if (bound.bottom > box.bottom) {
            box.bottom = bound.bottom;
        }
        previous = finish;
    }
    opened := 0;
    for (i = 0; i < len nodes; i++) {
        if (i % 128 == 0) {
            checkwork(work);
        }
        node := nodes[i];
        finish := cutpoint(node.x, node.y);
        if (node.verb == Move) {
            if (opened && !cutapart(paintnodebox(node, previous, origin), box)) {
                return 0;
            }
            opened = 1;
            origin = finish;
        } else {
            if (node.verb == Close) {
                opened = 0;
                finish = origin;
            }
            if (!cutapart(paintnodebox(node, previous, finish), box)) {
                return 0;
            }
        }
        previous = finish;
    }
    return !opened || cutapart(paintnodebox(Node(Close, 0, 0, 0, 0, 0, 0), previous, origin), box);
}


# EN: Preserve the old SVG and append an opposite-winding brush contour if
# the entire stamp has a one-cell margin inside old. Crossing edges and
# ambiguous winding return nil, asking the caller to trace the cut coverage.
# RU: Сохранить старый SVG и добавить обратный обход кисти, если весь отпечаток
# отстоит от границы old на одну ячейку. При пересечениях и неоднозначном
# обходе вернуть nil, чтобы вызывающий построил контур оставшегося покрытия.
painthole(nodes, brush: array of Node, footprint, old: array of byte, work: ref Work): array of Node
{
    if (len nodes + len brush > MaxNodes || !paintpunchclear(nodes, brush, work)) {
        return nil;
    }
    for (i := 0; i < GridBytes; i++) {
        if (i % 512 == 0) {
            checkwork(work);
        }
        bits := int footprint[i];
        if (bits == 0) {
            continue;
        }
        for (bit := 0; bit < 8; bit++) {
            if ((bits & (1 << bit)) != 0) {
                cell := 8*i + bit;
                x := cell % GridWidth;
                y := cell / GridWidth;
                for (dy := -1; dy <= 1; dy++) {
                    for (dx := -1; dx <= 1; dx++) {
                        if (!covered(old, x+dx, y+dy)) {
                            return nil;
                        }
                    }
                }
            }
        }
    }
    (unused, winding) := paintmeasure(nodes, Point(brush[0].x, brush[0].y), work);
    if (winding != 1 && winding != -1) {
        return nil;
    }
    result := array[len nodes + len brush] of Node;
    result[0:] = nodes;
    at := len nodes;
    if (winding < 0) {
        result[at:] = brush;
        return result;
    }
    last := len brush-2;
    result[at++] = Node(Move, brush[last].x, brush[last].y, 0, 0, 0, 0);
    for (i = last; i > 0; i--) {
        oldnode := brush[i];
        target := brush[i-1];
        result[at++] = Node(oldnode.verb, target.x, target.y, oldnode.c2x, oldnode.c2y, oldnode.cx, oldnode.cy);
    }
    result[at] = Node(Close, 0, 0, 0, 0, 0, 0);
    return result;
}


# EN: Group a traced mask's negative loops with their smallest enclosing
# positive loop. Positive islands are individually selectable. A supplied exact
# path retains its original geometry. Nil means contour or shape budget refusal.
# RU: Присоединить отрицательные обходы к наименьшему охватывающему внешнему
# контуру. Отдельные островки можно выделять по одному. Готовый точный путь
# сохраняет геометрию. Nil означает превышение бюджета или ошибку обхода.
paintislands(color: int, mask: array of byte, exact: array of Node, work: ref Work): array of ref Shape
{
    if (exact != nil) {
        return array[] of {ref Shape(color, 0, 1, exact, mask, 0, nil)};
    }
    nodes := contours(mask, work);
    return paintregions(color, mask, nodes, nil, work);
}


# EN: Split positive regions from copied vector or traced coverage contours.
# Assign each hole to its enclosing region. A nil mask is computed lazily.
# RU: Разделить внешние области векторных контуров или контуров маски.
# Присоединить отверстия к своим областям. Нулевая маска строится по запросу.
paintregions(color: int, mask: array of byte, nodes: array of Node, modes: array of int, work: ref Work): array of ref Shape
{
    if (len nodes < 2) {
        return nil;
    }
    loops := array[len nodes/3 + 1] of PaintLoop;
    pos := 0;
    count := 0;
    islands := 0;
    for (i := 0; i < len nodes; i++) {
        if (nodes[i].verb == Close) {
            path := nodes[pos:i+1];
            (area, unused) := paintmeasure(path, Point(0, 0), work);
            if (area > big 0) {
                islands++;
            }
            style: array of int = nil;
            if (modes != nil) {
                style = modes[pos:i+1];
            }
            loops[count++] = PaintLoop(path, style, area, -1);
            pos = i+1;
        }
    }
    if (islands <= 1) {
        return array[] of {ref Shape(color, 0, 1, nodes, mask, 0, modes)};
    }
    if (islands > MaxShapes) {
        return nil;
    }
    for (i = 0; i < count; i++) {
        if (loops[i].area < big 0) {
            origin := loops[i].nodes[0];
            parent := -1;
            for (j := 0; j < count; j++) {
                if (loops[j].area <= big 0 || (parent >= 0 && loops[j].area >= loops[parent].area)) {
                    continue;
                }
                (unused, winding) := paintmeasure(loops[j].nodes, Point(origin.x, origin.y), work);
                if (winding != 0) {
                    parent = j;
                }
            }
            if (parent < 0) {
                return nil;
            }
            loops[i].owner = parent;
        }
    }
    shapes := array[islands] of ref Shape;
    out := 0;
    for (i = 0; i < count; i++) {
        if (loops[i].area <= big 0) {
            continue;
        }
        length := len loops[i].nodes;
        for (j := 0; j < count; j++) {
            if (loops[j].owner == i) {
                length += len loops[j].nodes;
            }
        }
        region := array[length] of Node;
        region[0:] = loops[i].nodes;
        at := len loops[i].nodes;
        joins: array of int = nil;
        if (modes != nil) {
            joins = array[length] of int;
            joins[0:] = loops[i].modes;
        }
        for (j = 0; j < count; j++) {
            if (loops[j].owner == i) {
                region[at:] = loops[j].nodes;
                if (joins != nil) {
                    joins[at:] = loops[j].modes;
                }
                at += len loops[j].nodes;
            }
        }
        shapes[out++] = ref Shape(color, 0, 1, region, nil, 0, joins);
    }
    return shapes;
}
