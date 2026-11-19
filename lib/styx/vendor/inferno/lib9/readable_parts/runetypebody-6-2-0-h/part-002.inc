/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of runetypebody-6.2.0.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

    0x1ff9,  1048448, 0x1ffa,  0x1ffb,  1048450, 0x2160,  0x216f,  1048592, 0x24b6,  0x24cf,  1048602, 0x2c00,  0x2c2e,
    1048624, 0x2c7e,  0x2c7f,  1037761, 0xff21,  0xff3a,  1048608, 0x10400, 0x10427, 1048616,
};

static Rune __tolowerp[] = {
    0x0100, 0x012e, 1048577, 0x0132, 0x0136, 1048577, 0x0139, 0x0147, 1048577, 0x014a, 0x0176, 1048577,
    0x017b, 0x017d, 1048577, 0x01a2, 0x01a4, 1048577, 0x01b3, 0x01b5, 1048577, 0x01cd, 0x01db, 1048577,
    0x01de, 0x01ee, 1048577, 0x01f8, 0x021e, 1048577, 0x0222, 0x0232, 1048577, 0x0248, 0x024e, 1048577,
    0x0370, 0x0372, 1048577, 0x03d8, 0x03ee, 1048577, 0x0460, 0x0480, 1048577, 0x048a, 0x04be, 1048577,
    0x04c3, 0x04cd, 1048577, 0x04d0, 0x0526, 1048577, 0x1e00, 0x1e94, 1048577, 0x1ea0, 0x1efe, 1048577,
    0x1f59, 0x1f5f, 1048568, 0x2c67, 0x2c6b, 1048577, 0x2c80, 0x2ce2, 1048577, 0x2ceb, 0x2ced, 1048577,
    0xa640, 0xa66c, 1048577, 0xa680, 0xa696, 1048577, 0xa722, 0xa72e, 1048577, 0xa732, 0xa76e, 1048577,
    0xa779, 0xa77b, 1048577, 0xa780, 0xa786, 1048577, 0xa790, 0xa792, 1048577, 0xa7a0, 0xa7a8, 1048577,
};

static Rune __tolowers[] = {
    0x0130,  1048377, 0x0178,  1048455, 0x0179,  1048577, 0x0181,  1048786, 0x0182,  1048577, 0x0184,  1048577, 0x0186,
    1048782, 0x0187,  1048577, 0x018b,  1048577, 0x018e,  1048655, 0x018f,  1048778, 0x0190,  1048779, 0x0191,  1048577,
    0x0193,  1048781, 0x0194,  1048783, 0x0196,  1048787, 0x0197,  1048785, 0x0198,  1048577, 0x019c,  1048787, 0x019d,
    1048789, 0x019f,  1048790, 0x01a0,  1048577, 0x01a6,  1048794, 0x01a7,  1048577, 0x01a9,  1048794, 0x01ac,  1048577,
    0x01ae,  1048794, 0x01af,  1048577, 0x01b7,  1048795, 0x01b8,  1048577, 0x01bc,  1048577, 0x01c4,  1048578, 0x01c5,
    1048577, 0x01c7,  1048578, 0x01c8,  1048577, 0x01ca,  1048578, 0x01cb,  1048577, 0x01f1,  1048578, 0x01f2,  1048577,
    0x01f4,  1048577, 0x01f6,  1048479, 0x01f7,  1048520, 0x0220,  1048446, 0x023a,  1059371, 0x023b,  1048577, 0x023d,
    1048413, 0x023e,  1059368, 0x0241,  1048577, 0x0243,  1048381, 0x0244,  1048645, 0x0245,  1048647, 0x0246,  1048577,
    0x0376,  1048577, 0x0386,  1048614, 0x038c,  1048640, 0x03cf,  1048584, 0x03f4,  1048516, 0x03f7,  1048577, 0x03f9,
    1048569, 0x03fa,  1048577, 0x04c0,  1048591, 0x04c1,  1048577, 0x10c7,  1055840, 0x10cd,  1055840, 0x1e9e,  1040961,
    0x1fbc,  1048567, 0x1fcc,  1048567, 0x1fec,  1048569, 0x1ffc,  1048567, 0x2126,  1041059, 0x212a,  1040193, 0x212b,
    1040314, 0x2132,  1048604, 0x2183,  1048577, 0x2c60,  1048577, 0x2c62,  1037833, 0x2c63,  1044762, 0x2c64,  1037849,
    0x2c6d,  1037796, 0x2c6e,  1037827, 0x2c6f,  1037793, 0x2c70,  1037794, 0x2c72,  1048577, 0x2c75,  1048577, 0x2cf2,
    1048577, 0xa77d,  1013244, 0xa77e,  1048577, 0xa78b,  1048577, 0xa78d,  1006296, 0xa7aa,  1006268,
};

/* [standalone-contract:tolowerrune]
 * EN: tolowerrune: original POSIX lib9 runetypebody-6.2.0 support.
 * RU: tolowerrune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Rune; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune tolowerrune(Rune c)
{
    Rune *p;

    p = _runebsearch(c, __tolowerr, nelem(__tolowerr) / 3, 3);
    if (p && c >= p[0] && c <= p[1])
    {
        return c + p[2] - 1048576;
    }
    p = _runebsearch(c, __tolowerp, nelem(__tolowerp) / 3, 3);
    if (p && c >= p[0] && c <= p[1] && !((c - p[0]) & 1))
    {
        return c + p[2] - 1048576;
    }
    p = _runebsearch(c, __tolowers, nelem(__tolowers) / 2, 2);
    if (p && c == p[0])
    {
        return c + p[1] - 1048576;
    }
    return c;
}

static Rune __totitler[] = {
    0x0061,  0x007a,  1048544, 0x00e0,  0x00f6,  1048544, 0x00f8,  0x00fe,  1048544, 0x023f,  0x0240,  1059391, 0x0256,
    0x0257,  1048371, 0x028a,  0x028b,  1048359, 0x037b,  0x037d,  1048706, 0x03ad,  0x03af,  1048539, 0x03b1,  0x03c1,
    1048544, 0x03c3,  0x03cb,  1048544, 0x03cd,  0x03ce,  1048513, 0x0430,  0x044f,  1048544, 0x0450,  0x045f,  1048496,
    0x0561,  0x0586,  1048528, 0x1f00,  0x1f07,  1048584, 0x1f10,  0x1f15,  1048584, 0x1f20,  0x1f27,  1048584, 0x1f30,
    0x1f37,  1048584, 0x1f40,  0x1f45,  1048584, 0x1f60,  0x1f67,  1048584, 0x1f70,  0x1f71,  1048650, 0x1f72,  0x1f75,
    1048662, 0x1f76,  0x1f77,  1048676, 0x1f78,  0x1f79,  1048704, 0x1f7a,  0x1f7b,  1048688, 0x1f7c,  0x1f7d,  1048702,
    0x1f80,  0x1f87,  1048584, 0x1f90,  0x1f97,  1048584, 0x1fa0,  0x1fa7,  1048584, 0x1fb0,  0x1fb1,  1048584, 0x1fd0,
    0x1fd1,  1048584, 0x1fe0,  0x1fe1,  1048584, 0x2170,  0x217f,  1048560, 0x24d0,  0x24e9,  1048550, 0x2c30,  0x2c5e,
    1048528, 0x2d00,  0x2d25,  1041312, 0xff41,  0xff5a,  1048544, 0x10428, 0x1044f, 1048536,
};

static Rune __totitlep[] = {
    0x0101,  0x012f,  1048575, 0x0133,  0x0137,  1048575, 0x013a,  0x0148,  1048575, 0x014b,  0x0177,  1048575, 0x017a,
    0x017e,  1048575, 0x0183,  0x0185,  1048575, 0x01a1,  0x01a5,  1048575, 0x01b4,  0x01b6,  1048575, 0x01cc,  0x01dc,
    1048575, 0x01df,  0x01ef,  1048575, 0x01f3,  0x01f5,  1048575, 0x01f9,  0x021f,  1048575, 0x0223,  0x0233,  1048575,
    0x0247,  0x024f,  1048575, 0x0371,  0x0373,  1048575, 0x03d9,  0x03ef,  1048575, 0x0461,  0x0481,  1048575, 0x048b,
    0x04bf,  1048575, 0x04c2,  0x04ce,  1048575, 0x04d1,  0x0527,  1048575, 0x1e01,  0x1e95,  1048575, 0x1ea1,  0x1eff,
    1048575, 0x1f51,  0x1f57,  1048584, 0x2c68,  0x2c6c,  1048575, 0x2c81,  0x2ce3,  1048575, 0x2cec,  0x2cee,  1048575,
    0xa641,  0xa66d,  1048575, 0xa681,  0xa697,  1048575, 0xa723,  0xa72f,  1048575, 0xa733,  0xa76f,  1048575, 0xa77a,
    0xa77c,  1048575, 0xa77f,  0xa787,  1048575, 0xa791,  0xa793,  1048575, 0xa7a1,  0xa7a9,  1048575,
};

static Rune __totitles[] = {
    0x00b5,  1049319, 0x00ff,  1048697, 0x0131,  1048344, 0x017f,  1048276, 0x0180,  1048771, 0x0188,  1048575, 0x018c,
    1048575, 0x0192,  1048575, 0x0195,  1048673, 0x0199,  1048575, 0x019a,  1048739, 0x019e,  1048706, 0x01a8,  1048575,
    0x01ad,  1048575, 0x01b0,  1048575, 0x01b9,  1048575, 0x01bd,  1048575, 0x01bf,  1048632, 0x01c4,  1048577, 0x01c6,
    1048575, 0x01c7,  1048577, 0x01c9,  1048575, 0x01ca,  1048577, 0x01dd,  1048497, 0x01f1,  1048577, 0x023c,  1048575,
    0x0242,  1048575, 0x0250,  1059359, 0x0251,  1059356, 0x0252,  1059358, 0x0253,  1048366, 0x0254,  1048370, 0x0259,
    1048374, 0x025b,  1048373, 0x0260,  1048371, 0x0263,  1048369, 0x0265,  1090856, 0x0266,  1090884, 0x0268,  1048367,
    0x0269,  1048365, 0x026b,  1059319, 0x026f,  1048365, 0x0271,  1059325, 0x0272,  1048363, 0x0275,  1048362, 0x027d,
    1059303, 0x0280,  1048358, 0x0283,  1048358, 0x0288,  1048358, 0x0289,  1048507, 0x028c,  1048505, 0x0292,  1048357,
    0x0345,  1048660, 0x0377,  1048575, 0x03ac,  1048538, 0x03c2,  1048545, 0x03cc,  1048512, 0x03d0,  1048514, 0x03d1,
    1048519, 0x03d5,  1048529, 0x03d6,  1048522, 0x03d7,  1048568, 0x03f0,  1048490, 0x03f1,  1048496, 0x03f2,  1048583,
    0x03f5,  1048480, 0x03f8,  1048575, 0x03fb,  1048575, 0x04cf,  1048561, 0x1d79,  1083908, 0x1d7d,  1052390, 0x1e9b,
    1048517, 0x1fb3,  1048585, 0x1fbe,  1041371, 0x1fc3,  1048585, 0x1fe5,  1048583, 0x1ff3,  1048585, 0x214e,  1048548,
    0x2184,  1048575, 0x2c61,  1048575, 0x2c65,  1037781, 0x2c66,  1037784, 0x2c73,  1048575, 0x2c76,  1048575, 0x2cf3,
    1048575, 0x2d27,  1041312, 0x2d2d,  1041312, 0xa78c,  1048575,
};

/* [standalone-contract:totitlerune]
 * EN: totitlerune: original POSIX lib9 runetypebody-6.2.0 support.
 * RU: totitlerune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Rune; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune totitlerune(Rune c)
{
    Rune *p;

    p = _runebsearch(c, __totitler, nelem(__totitler) / 3, 3);
    if (p && c >= p[0] && c <= p[1])
    {
        return c + p[2] - 1048576;
    }
    p = _runebsearch(c, __totitlep, nelem(__totitlep) / 3, 3);
    if (p && c >= p[0] && c <= p[1] && !((c - p[0]) & 1))
    {
        return c + p[2] - 1048576;
    }
    p = _runebsearch(c, __totitles, nelem(__totitles) / 2, 2);
    if (p && c == p[0])
    {
        return c + p[1] - 1048576;
    }
    return c;
}
