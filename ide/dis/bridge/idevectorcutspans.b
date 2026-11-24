# Copyright (c) 2026 bhgv
# EN: Prepare bounded exact subcurves for source edges that loop or turn back.
# RU: Подготавливать точные участки исходных кривых с петлями или возвратом.


# EN: Private geometry buffer used during input expansion.
# RU: Частный буфер при разбиении исходных сегментов.
CutExpansion: adt
{
    edges: array of CutCurve; # Copied spans, unused beyond used.
    used: int;                # Committed count, or -1 when expansion must decline.
    work: ref Work;           # Existing cancellation state.
};


# EN: Recover both interior parameters of a transverse cubic loop. For cubic
# coefficients a,b,c,d, equal positions at t,u give
# a*(t*t+t*u+u*u)+b*(t+u)+c=0. Verify the derived pair in page coordinates.
# RU: Найти оба внутренних параметра поперечной петли кубики. Равенство
# позиций при t,u даёт a*(t*t+t*u+u*u)+b*(t+u)+c=0. Найденная пара
# дополнительно проверяется в координатах листа.
cutloopspan(edge: CutCurve, math: Math): (real, real)
{
    if (edge.verb != Cubic) {
        return (0.0, 0.0);
    }
    p := cutsub(edge.b, edge.a);
    q := cutsub(edge.c, edge.b);
    r := cutsub(edge.d, edge.c);
    c := CutPoint(3.0*p.x, 3.0*p.y);
    b := CutPoint(3.0*(q.x-p.x), 3.0*(q.y-p.y));
    a := CutPoint(p.x-2.0*q.x+r.x, p.y-2.0*q.y+r.y);
    cross := cutcross(a, b);
    if (cross*cross <= 1.0e-20*cutdot(a, a)*cutdot(b, b)) {
        return (0.0, 0.0);
    }
    sum := cutcross(c, a)/cross;
    squares := -cutcross(c, b)/cross;
    discriminant := 4.0*squares-3.0*sum*sum;
    if (discriminant <= 1.0e-12) {
        return (0.0, 0.0);
    }
    distance := math->sqrt(discriminant);
    first := (sum-distance)*0.5;
    last := (sum+distance)*0.5;
    if (first <= 1.0e-8 || last >= 1.0-1.0e-8) {
        return (0.0, 0.0);
    }
    (one, ignored) := cuteval(edge, first);
    (other, unused) := cuteval(edge, last);
    if (!cutsamepoint(one, other)) {
        return (0.0, 0.0);
    }
    return (first, last);
}


# EN: Retain a copied exact interval once its chord is a monotone coordinate.
# No sample fit or rounding occurs during the subdivision.
# RU: Сохранить точный участок, когда проекция на хорду монотонна.
# Деление не подбирает кривую по отсчётам и не округляет координаты.
cutgrow(expansion: ref CutExpansion, edge: CutCurve, depth: int)
{
    if (expansion.used < 0) {
        return;
    }
    checkwork(expansion.work);
    axis := cutsub(edge.d, edge.a);
    if (cutdot(axis, axis) > 1.0e-12 && cutprojection(edge, axis)) {
        if (expansion.used == len expansion.edges) {
            expansion.used = -1;
            return;
        }
        expansion.edges[expansion.used++] = edge;
        return;
    }
    if (depth == 10) {
        expansion.used = -1;
        return;
    }
    (left, right) := cuthalf(edge);
    cutgrow(expansion, left, depth+1);
    cutgrow(expansion, right, depth+1);
}


# EN: Expand a borrowed original segment. For a cubic loop split at both
# visits and between them so the loop never becomes a zero-chord span.
# RU: Разбить исходный сегмент. Петлю делить в обоих посещениях точки
# пересечения и между ними, чтобы её участок не имел нулевой хорды.
cutexpand(expansion: ref CutExpansion, edge: CutCurve, math: Math)
{
    (start, end) := cutloopspan(edge, math);
    if (end == 0.0) {
        cutgrow(expansion, edge, 0);
        return;
    }
    middle := (start+end)*0.5;
    cutgrow(expansion, cutslice(edge, 0.0, start), 0);
    cutgrow(expansion, cutslice(edge, start, middle), 0);
    cutgrow(expansion, cutslice(edge, middle, end), 0);
    cutgrow(expansion, cutslice(edge, end, 1.0), 0);
}
