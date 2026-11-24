# Copyright (c) 2026 bhgv
# EN: Certify separated Bézier spans and point contacts on a common tangent.
# RU: Подтверждать разделение сегментов Безье и точечное касание по общей касательной.


# EN: Exact candidates for the extrema of the projected polynomial.
# RU: Кандидаты экстремумов проекции полиномиального сегмента.
CutExtrema: adt
{
    at: array of (real, real); # Curve parameter and scalar projection.
    lo: real;                  # Minimum projection across candidates.
    hi: real;                  # Maximum projection across candidates.
    first: real;               # Curve parameter at the chosen minimum.
    last: real;                # Curve parameter at the chosen maximum.
};


# EN: Compute the scalar projection of one borrowed cubic, including interior
# extrema. Translating both curves is unnecessary at bounded page coordinates.
# RU: Вычислить проекцию кубики с внутренними экстремумами. При ограниченных
# координатах листа общий перенос не нужен.
cutextrema(edge: CutCurve, normal: CutPoint, math: Math): CutExtrema
{
    a := cutdot(edge.a, normal);
    b := cutdot(edge.b, normal);
    c := cutdot(edge.c, normal);
    d := cutdot(edge.d, normal);
    poly := array[] of {a, 3.0*(b-a), 3.0*(c-2.0*b+a), d-3.0*c+3.0*b-a};
    bounds := cutderivative(poly, math);
    at := array[len bounds] of (real, real);
    lo := a;
    hi := a;
    first := 0.0;
    last := 0.0;
    for (i := 0; i < len bounds; i++) {
        value := cutpoly(poly, bounds[i]);
        at[i] = (bounds[i], value);
        if (value < lo) {
            lo = value;
            first = bounds[i];
        }
        if (value > hi) {
            hi = value;
            last = bounds[i];
        }
    }
    return CutExtrema(at, lo, hi, first, last);
}


# EN: Try the unit normal as a separating line. A contact is accepted only
# if both supporting parameters produce the same two-dimensional point.
# RU: Проверить единичную нормаль разделяющей прямой. Касание принимается,
# только если обе опорные точки совпадают в двух координатах.
cutseparates(pair: CutPair, normal: CutPoint, contacts: ref CutContacts): int
{
    a := cutextrema(pair.a, normal, contacts.math);
    b := cutextrema(pair.b, normal, contacts.math);
    gap := b.lo-a.hi;
    tolerance := 2.0e-7;
    if (gap > tolerance) {
        return 1;
    }
    if (gap < -tolerance) {
        return 0;
    }
    found := 0;
    for (i := 0; i < len a.at && contacts.search.count >= 0; i++) {
        (t, high) := a.at[i];
        if (high < a.hi-tolerance) {
            continue;
        }
        (p, ignored) := cuteval(pair.a, t);
        for (j := 0; j < len b.at && contacts.search.count >= 0; j++) {
            (u, low) := b.at[j];
            if (low <= b.lo+tolerance) {
                (q, unused) := cuteval(pair.b, u);
                if (cutsamepoint(p, q)) {
                    cuthitadd(contacts.search, pair, t, u, cutlerp(p, q, 0.5));
                    found = 1;
                }
            }
        }
    }
    return found;
}


# EN: Try both directions perpendicular to a sampled derivative.
# RU: Проверить оба направления нормали к производной.
cutsupportnormal(pair: CutPair, derivative: CutPoint, contacts: ref CutContacts): int
{
    distance2 := cutdot(derivative, derivative);
    if (distance2 < 1.0e-12) {
        return 0;
    }
    scale := 1.0/contacts.math->sqrt(distance2);
    normal := CutPoint(-derivative.y*scale, derivative.x*scale);
    return cutseparates(pair, normal, contacts) ||
        cutseparates(pair, CutPoint(-normal.x, -normal.y), contacts);
}


# EN: Measure the support gap at one candidate angle for normal discovery.
# This score alone never establishes a contact.
# RU: Измерить разрыв опорных проекций при поиске нормали. Само значение
# разрыва не подтверждает касание.
cutsupportscore(pair: CutPair, angle: real, math: Math): real
{
    normal := CutPoint(math->cos(angle), math->sin(angle));
    a := cutextrema(pair.a, normal, math);
    b := cutextrema(pair.b, normal, math);
    return b.lo-a.hi;
}


# EN: Measure the change of the support gap with angle using the two
# current extreme positions. This avoids subtracting nearly equal scores
# when the separating tangent is already close.
# RU: Измерить изменение опорного разрыва по двум текущим экстремумам.
# Это избавляет от вычитания близких значений рядом с общей касательной.
cutsupportslope(pair: CutPair, angle: real, math: Math): real
{
    normal := CutPoint(math->cos(angle), math->sin(angle));
    a := cutextrema(pair.a, normal, math);
    b := cutextrema(pair.b, normal, math);
    (p, ignored) := cuteval(pair.a, a.last);
    (q, unused) := cuteval(pair.b, b.first);
    return cutcross(normal, cutsub(q, p));
}


# EN: Explore a bounded angular neighbourhood of the best coarse direction.
# A found normal still passes full projected-extrema and position checks.
# RU: Уточнить угол около лучшего направления на грубой сетке. Найденная
# нормаль всё равно проходит проверку экстремумов и координат.
cutsupportsearch(pair: CutPair, contacts: ref CutContacts): int
{
    math := contacts.math;
    step := 0.7853981633974483;
    angle := 0.0;
    score := cutsupportscore(pair, 0.0, math);
    for (i := 1; i < 8; i++) {
        at := step*real i;
        value := cutsupportscore(pair, at, math);
        if (value > score) {
            angle = at;
            score = value;
        }
    }
    lo := angle-step;
    hi := angle+step;
    if (cutsupportslope(pair, lo, math) < 0.0 || cutsupportslope(pair, hi, math) > 0.0) {
        return 0;
    }
    for (i = 0; i < 46; i++) {
        if (i % 8 == 0) {
            checkwork(contacts.search.work);
        }
        middle := (lo+hi)*0.5;
        if (cutsupportslope(pair, middle, math) > 0.0) {
            lo = middle;
        } else {
            hi = middle;
        }
    }
    angle = (lo+hi)*0.5;
    return cutseparates(pair, CutPoint(math->cos(angle), math->sin(angle)), contacts);
}


# EN: Recover a pair whose ordinary transverse search was inconclusive.
# Charge both the failed search and a bounded support budget to this job.
# RU: Повторно проверить пару после неудачного поиска поперечного корня.
# Неудачный поиск и ограниченный поиск опоры расходуют общий бюджет.
cutsupport(pair: CutPair, contacts: ref CutContacts): int
{
    search := contacts.search;
    if (search.budget < 192) {
        return 0;
    }
    search.budget -= 192;
    if (cutsupportnormal(pair, CutPoint(1.0, 0.0), contacts) ||
        cutsupportnormal(pair, CutPoint(0.0, 1.0), contacts)) {
        return 1;
    }
    for (i := 0; i <= 4; i++) {
        t := real i/4.0;
        (ap, ad) := cuteval(pair.a, t);
        (bp, bd) := cuteval(pair.b, t);
        if (cutsupportnormal(pair, ad, contacts) || cutsupportnormal(pair, bd, contacts)) {
            return 1;
        }
    }
    return cutsupportsearch(pair, contacts);
}
