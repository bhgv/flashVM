# Copyright (c) 2026 bhgv
# EN: Extracted raw acceptance oracle; preserve instruction order and literals.
# RU: Извлечённый raw-oracle приёмки; сохранить порядок и литералы.
implement ArrayLength;
include "sys.m";
ArrayLength: module {init: fn();};
Pair: adt { first, second: int; };
fresh(): array of Pair {
    return array[] of {Pair(19, 23)};
}
sparse(value: int): int {
    case value {
    -42 => return 11;
    100 to 105 => return 22;
    * => return 33;
    }
}
large(value: big): int {
    case value {
    big 9007199254740993 => return 19;
    big -9007199254740993 => return 23;
    * => return 0;
    }
}
strings(value: string): int {
    case value {
    "2.5" => return 29;
    "Ж" => return 31;
    * => return 0;
    }
}
dense(value: int): int {
    case value {
    0 => return 2;
    1 => return 5;
    2 => return 11;
    3 => return 17;
    4 => return 23;
    5 => return 31;
    6 => return 41;
    7 => return 47;
    * => return 97;
    }
}
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN array length\n");
    for (i := 0; i < 32; i++) {
        b := array[i] of byte;
        w := array[i] of int;
        r := array[i] of string;
        if (len b != i || len w != i || len r != i) {
            raise "array length mismatch";
        }
        if (i != 0) {
            at := i - 1;
            b[at] = byte i;
            w[at] = i + 7;
            r[at] = "guarded";
            wide := array[1] of big;
            floats := array[1] of real;
            zero := at - at;
            pair := fresh()[zero];
            if (pair.first != 19 || pair.second != 23) {
                raise "temporary array index lost its owner";
            }
            wide[zero] = big i;
            floats[zero] = real i;
            if (wide[zero] != big i || floats[zero] != real i) {
                raise "wide array index mismatch";
            }
            if (int b[at] != i || w[at] != i + 7 || r[at] != "guarded") {
                raise "array index mismatch";
            }
        }
    }
    source := array[] of {"left", "middle", "right"};
    copy := source[-2:-1];
    copy[-1] = "copy";
    source[-2:] = array[] of {"new", "tail"};
    if (len copy != 2 || copy[-1] != "copy" || source[-2] != "new" || source[-1] != "tail") {
        raise "guarded mapped slice mismatch";
    }
    mapped := array[] of {"a", "b", "c", "d"};
    mapped[1:] = mapped[0:3];
    if (mapped[0] != "a" || mapped[1] != "a" || mapped[2] != "b" || mapped[3] != "c") {
        raise "right overlapping managed copy";
    }
    mapped[0:] = mapped[1:4];
    if (mapped[0] != "a" || mapped[1] != "b" || mapped[2] != "c" || mapped[3] != "c") {
        raise "left overlapping managed copy";
    }
    view := mapped[0:4];
    nested := view[1:3];
    if (len view != 4 || len nested != 2 || nested[0] != "b" || nested[1] != "c") {
        raise "nested view lost its backing owner";
    }
    scalar := array[] of {11, 22, 33, 44};
    scalar[1:] = scalar[0:3];
    scalar = scalar[1:3];
    if (len scalar != 2 || scalar[0] != 11 || scalar[1] != 22) {
        raise "scalar copy and in-place view narrowing";
    }
    relative_scalar := scalar[-2:-1];
    relative_scalar[0] = 99;
    if (len relative_scalar != 2 || relative_scalar[1] != 22 || scalar[0] != 11) {
        raise "relative scalar slice must publish an independent final copy";
    }
    samples := array[] of {"-42", "9007199254740993", "2.5", "Ж"};
    word := int samples[0];
    wide := big samples[1];
    realvalue := real samples[2];
    if (sparse(word) != 11 || sparse(0) != 33 || large(wide) != 19 ||
        large(-wide) != 23 || strings(samples[2]) != 29 || strings(samples[3]) != 31 ||
        dense(len samples) != 23 || dense(word) != 97) {
        raise "guarded case and goto selectors";
    }
    capacity := len samples - 2;
    messages := chan[capacity] of int;
    pairs := chan[capacity] of (int, int);
    records := chan[capacity] of (int, string);
    byteschan := chan[capacity] of byte;
    bigchan := chan[capacity] of big;
    realchan := chan[capacity] of real;
    stringchan := chan[capacity] of string;
    records <-= (22, samples[3]);
    records <-= (23, samples[0]);
    (tag, text) := <-records;
    (nexttag, nexttext) := <-records;
    messages <-= 19;
    messages <-= 24;
    pairs <-= (20, 21);
    pairs <-= (25, 26);
    message := <-messages;
    nextmessage := <-messages;
    (left, right) := <-pairs;
    (nextleft, nextright) := <-pairs;
    byteschan <-= byte 7;
    bigchan <-= wide;
    realchan <-= realvalue;
    stringchan <-= samples[3];
    if (int <-byteschan != 7 || <-bigchan != wide ||
        <-realchan != realvalue || <-stringchan != "Ж") {
        raise "typed channel constructor replay";
    }
    if (message != 19 || left != 20 || right != 21 || tag != 22 || text != "Ж" ||
        nexttag != 23 || nexttext != "-42" || nextmessage != 24 ||
        nextleft != 25 || nextright != 26) {
        raise "guarded channel capture changed buffered transfer";
    }
    altvalue := 0;
    messages <-= 31;
    {
        alt {
            altvalue = <-messages => {}
        }
    } exception failure {
        "out of memory: heap" =>
            alt {
                altvalue = <-messages => {}
            }
        * => raise;
    }
    if (altvalue != 31) {
        raise "blocking ALT header replay";
    }
    alt {
        <-messages => raise "unexpected value after ALT";
        * => altvalue = 37;
    }
    if (altvalue != 37) {
        raise "nonblocking ALT header replay";
    }
    encoded := array of byte samples[3];
    decoded := string encoded;
    if (word != -42 || wide != big 9007199254740993 || realvalue != 2.5 ||
        string word != "-42" || string wide != "9007199254740993" ||
        real string realvalue != 2.5 || decoded != "Ж" || len encoded != 2) {
        raise "guarded string conversion mismatch";
    }
    checkwide := big 0;
    {
        checkwide = big samples[3];
    } exception failure {
        "test UTF-8 view refusal" => checkwide = big 19;
        * => raise;
    }
    if (checkwide != big 19 && checkwide != big 0) {
        raise "string conversion refusal handling";
    }
    checkwide = big samples[1];
    if (checkwide != wide) {
        raise "string conversion failed after refusal";
    }
    narrowtext, widetext: string;
    for (step := 0; step < 3; step++) {
        narrowtext = string (word + step);
        widetext = string (wide + big step);
        if (int narrowtext != word + step || big widetext != wide + big step) {
            raise "integer string publication mismatch";
        }
    }
    realtext, asciitext: string;
    asciibytes: array of byte;
    for (pass := 0; pass < 3; pass++) {
        realtext = string (realvalue + real pass);
        asciibytes = array of byte samples[0];
        asciitext = string asciibytes;
        if (real realtext != realvalue + real pass || asciitext != samples[0]) {
            raise "real and byte string publication mismatch";
        }
    }
    n: array of byte;
    if (len n != 0) {
        raise "nil length mismatch";
    }
    sys->print("PASS array length32\n");
}
