# Copyright (c) 2026 bhgv
# EN: Real-valued Bézier segments for the private, bounded brush boolean.
# RU: Вещественные сегменты Безье для ограниченных операций кисти.


# EN: A point or vector in logical page coordinates.
# RU: Точка или вектор в координатах листа.
CutPoint: adt
{
    x: real; # Horizontal component.
    y: real; # Vertical component.
};


# EN: One directed Bézier. Lines and quadratics are degree-elevated internally.
# RU: Направленная Безье. Степень отрезков и квадратичных кривых повышена.
CutCurve: adt
{
    verb: int;  # Original SVG command, restored on output.
    a: CutPoint; # Start.
    b: CutPoint; # First cubic control.
    c: CutPoint; # Second cubic control.
    d: CutPoint; # End.
};


# EN: Convex hull bounds in page coordinates, including both controls.
# RU: Границы выпуклой оболочки с обеими ручками в координатах листа.
CutBox: adt
{
    left: real;   # Minimum X.
    top: real;    # Minimum Y.
    right: real;  # Maximum X.
    bottom: real; # Maximum Y.
};


# EN: Copy a page coordinate into real arithmetic before subdivision.
# RU: Перевести координату листа в вещественный формат перед делением.
cutpoint(x, y: int): CutPoint
{
    return CutPoint(real x, real y);
}


# EN: Subtract two copied points, returning the directed vector b to a.
# RU: Вычесть две точки, вернув вектор от b к a.
cutsub(a, b: CutPoint): CutPoint
{
    return CutPoint(a.x-b.x, a.y-b.y);
}


# EN: Dot product of copied page-space vectors.
# RU: Скалярное произведение векторов в координатах листа.
cutdot(a, b: CutPoint): real
{
    return a.x*b.x+a.y*b.y;
}


# EN: Signed two-dimensional cross product of copied vectors.
# RU: Знаковое векторное произведение на плоскости.
cutcross(a, b: CutPoint): real
{
    return a.x*b.y-a.y*b.x;
}


# EN: Offset a copied point by scale times a copied direction.
# RU: Сдвинуть точку на заданный множитель направления.
cutoffset(p, v: CutPoint, scale: real): CutPoint
{
    return CutPoint(p.x+scale*v.x, p.y+scale*v.y);
}


# EN: Interpolate two copied points at t between zero and one.
# RU: Интерполировать две точки с параметром t от нуля до единицы.
cutlerp(a, b: CutPoint, t: real): CutPoint
{
    return cutoffset(a, cutsub(b, a), t);
}


# EN: Evaluate a copied cubic and its derivative at path parameter t.
# RU: Вычислить точку кубической кривой и её производную при параметре t.
cuteval(edge: CutCurve, t: real): (CutPoint, CutPoint)
{
    ab := cutlerp(edge.a, edge.b, t);
    bc := cutlerp(edge.b, edge.c, t);
    cd := cutlerp(edge.c, edge.d, t);
    left := cutlerp(ab, bc, t);
    right := cutlerp(bc, cd, t);
    return (cutlerp(left, right, t), cutoffset(CutPoint(0.0, 0.0), cutsub(right, left), 3.0));
}


# EN: Split a cubic in half for hull isolation. Both copies retain the verb.
# RU: Разделить кривую пополам для уточнения оболочки. Команда сохраняется.
cuthalf(edge: CutCurve): (CutCurve, CutCurve)
{
    ab := cutlerp(edge.a, edge.b, 0.5);
    bc := cutlerp(edge.b, edge.c, 0.5);
    cd := cutlerp(edge.c, edge.d, 0.5);
    left := cutlerp(ab, bc, 0.5);
    right := cutlerp(bc, cd, 0.5);
    middle := cutlerp(left, right, 0.5);
    return (CutCurve(edge.verb, edge.a, ab, left, middle),
            CutCurve(edge.verb, middle, right, cd, edge.d));
}


# EN: Restrict a cubic to [lo, hi] using its endpoint positions and derivatives.
# Preserve untouched controls exactly. Callers may reverse the returned value.
# RU: Выделить отрезок параметров [lo, hi] по позициям и касательным концов.
# Неразрезанные ручки сохраняются. Результат можно обратить.
cutslice(edge: CutCurve, lo, hi: real): CutCurve
{
    if (lo == 0.0 && hi == 1.0) {
        return edge;
    }
    (a, va) := cuteval(edge, lo);
    (d, vd) := cuteval(edge, hi);
    scale := (hi-lo)/3.0;
    return CutCurve(edge.verb, a, cutoffset(a, va, scale), cutoffset(d, vd, -scale), d);
}


# EN: Reverse one directed value without reallocating the source contour.
# RU: Обратить один сегмент без выделения или изменения исходного контура.
cutreverse(edge: CutCurve): CutCurve
{
    return CutCurve(edge.verb, edge.d, edge.c, edge.b, edge.a);
}


# EN: Bound a copied curve by the axis-aligned box of its control hull.
# RU: Ограничить кривую прямоугольником её управляющих точек.
cutbounds(edge: CutCurve): CutBox
{
    box := CutBox(edge.a.x, edge.a.y, edge.a.x, edge.a.y);
    points := array[] of {edge.b, edge.c, edge.d};
    for (i := 0; i < len points; i++) {
        p := points[i];
        if (p.x < box.left) {
            box.left = p.x;
        }
        if (p.x > box.right) {
            box.right = p.x;
        }
        if (p.y < box.top) {
            box.top = p.y;
        }
        if (p.y > box.bottom) {
            box.bottom = p.y;
        }
    }
    return box;
}


# EN: Test strict box separation before descending to smaller hulls.
# RU: Проверить разделение прямоугольников перед делением оболочек.
cutapart(a, b: CutBox): int
{
    return a.right < b.left || b.right < a.left || a.bottom < b.top || b.bottom < a.top;
}


# EN: Sum the dimensions of a box for choosing which hull to bisect.
# RU: Сложить размеры оболочки для выбора делимого сегмента.
cutextent(box: CutBox): real
{
    return box.right-box.left+box.bottom-box.top;
}


# EN: Convert a borrowed single closed path to clockwise segment values.
# Nil leaves compound, tiny or overly complex contours on the existing path.
# RU: Перевести один замкнутый путь в сегменты с обходом по часовой стрелке.
# Nil оставляет составные, вырожденные и слишком сложные контуры старому пути.
cutcurves(nodes: array of Node, work: ref Work): array of CutCurve
{
    if (len nodes < 4 || len nodes > 98 || nodes[0].verb != Move || nodes[len nodes-1].verb != Close) {
        return nil;
    }
    result := array[len nodes-1] of CutCurve;
    start := cutpoint(nodes[0].x, nodes[0].y);
    a := start;
    count := 0;
    for (i := 1; i < len nodes; i++) {
        node := nodes[i];
        d := cutpoint(node.x, node.y);
        verb := node.verb;
        if (verb == Close && i == len nodes-1) {
            d = start;
            verb = Segment;
        }
        b := cutlerp(a, d, 1.0/3.0);
        c := cutlerp(a, d, 2.0/3.0);
        case verb {
        Quadratic =>
            control := cutpoint(node.cx, node.cy);
            b = cutlerp(a, control, 2.0/3.0);
            c = cutlerp(d, control, 2.0/3.0);
        Cubic =>
            b = cutpoint(node.cx, node.cy);
            c = cutpoint(node.c2x, node.c2y);
        Segment =>
            if (a.x == d.x && a.y == d.y) {
                continue;
            }
        * =>
            return nil;
        }
        if (a.x == d.x && a.y == d.y) {
            return nil;
        }
        result[count++] = CutCurve(verb, a, b, c, d);
        a = d;
    }
    (area, ignored) := paintmeasure(nodes, Point(0, 0), work);
    if (count < 2 || area == big 0) {
        return nil;
    }
    if (area < big 0) {
        for (i = 0; i < count/2; i++) {
            back := count-1-i;
            (result[i], result[back]) = (cutreverse(result[back]), cutreverse(result[i]));
        }
        if (count % 2 != 0) {
            result[count/2] = cutreverse(result[count/2]);
        }
    }
    return result[0:count];
}


# EN: Serialize one retained subcurve, rounding only the released SVG values.
# The supplied endpoint is shared by both incident curves at an intersection.
# RU: Сохранить участок кривой, округляя координаты только для SVG.
# Переданная конечная точка общая для обеих кривых на пересечении.
cutnode(edge: CutCurve, finish: CutPoint): Node
{
    x := int finish.x;
    y := int finish.y;
    if (edge.verb == Segment) {
        return Node(Segment, x, y, 0, 0, 0, 0);
    }
    if (edge.verb == Quadratic) {
        control := cutoffset(edge.a, cutsub(edge.b, edge.a), 1.5);
        return Node(Quadratic, x, y, int control.x, int control.y, 0, 0);
    }
    return Node(Cubic, x, y, int edge.b.x, int edge.b.y, int edge.c.x, int edge.c.y);
}
