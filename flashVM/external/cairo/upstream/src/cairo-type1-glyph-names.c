/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2006 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 */

#include "cairoint.h"

#if CAIRO_HAS_FONT_SUBSET

#include "cairo-type1-private.h"
#include "cairo-scaled-font-subsets-private.h"

#if 0
/*
 * The three tables that follow are generated using this perl code:
 */

@ps_standard_encoding = (
	#   0
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	#  16
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	#  32
	"space",	"exclam",	"quotedbl",	"numbersign",
	"dollar",	"percent",	"ampersand",	"quoteright",
	"parenleft",	"parenright",	"asterisk",	"plus",
	"comma",	"hyphen",	"period",	"slash",
	#  48
	"zero",		"one",		"two",		"three",
	"four",		"five",		"six",		"seven",
	"eight",	"nine",		"colon",	"semicolon",
	"less",		"equal",	"greater",	"question",
	#  64
	"at",		"A",		"B",		"C",
	"D",		"E",		"F",		"G",
	"H",		"I",		"J",		"K",
	"L",		"M",		"N",		"O",
	#  80
	"P",		"Q",		"R",		"S",
	"T",		"U",		"V",		"W",
	"X",		"Y",		"Z",		"bracketleft",
	"backslash",	"bracketright",	"asciicircum",	"underscore",
	#  96
	"quoteleft",	"a",		"b",		"c",
	"d",		"e",		"f",		"g",
	"h",		"i",		"j",		"k",
	"l",		"m",		"n",		"o",
	# 112
	"p",		"q",		"r",		"s",
	"t",		"u",		"v",		"w",
	"x",		"y",		"z",		"braceleft",
	"bar",		"braceright",	"asciitilde",	NULL,
	# 128
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	# 144
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	# 160
	NULL,		"exclamdown",	"cent",		"sterling",
	"fraction",	"yen",		"florin",	"section",
	"currency",	"quotesingle",	"quotedblleft",	"guillemotleft",
	"guilsinglleft","guilsinglright","fi",		"fl",
	# 176
	NULL,		"endash",	"dagger",	"daggerdbl",
	"periodcentered",NULL,		"paragraph",	"bullet",
	"quotesinglbase","quotedblbase","quotedblright","guillemotright",
	"ellipsis",	"perthousand",	NULL,		"questiondown",
	# 192
	NULL,		"grave",	"acute",	"circumflex",
	"tilde",	"macron",	"breve",	"dotaccent",
	"dieresis",	NULL,		"ring",		"cedilla",
	NULL,		"hungarumlaut",	"ogonek",	"caron",
	# 208
	"emdash",	NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	# 224
	NULL,		"AE",		NULL,		"ordfeminine",
	NULL,		NULL,		NULL,		NULL,
	"Lslash",	"Oslash",	"OE",		"ordmasculine",
	NULL,		NULL,		NULL,		NULL,
	# 240
	NULL,		"ae",		NULL,		NULL,
	NULL,		"dotlessi",	NULL,		NULL,
	"lslash",	"oslash",	"oe",		"germandbls",
	NULL,		NULL,		NULL,		NULL
	);

@winansi_encoding = (
	#   0
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	#  16
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	#  32
	"space",	"exclam",	"quotedbl",	"numbersign",
	"dollar",	"percent",	"ampersand",	"quotesingle",
	"parenleft",	"parenright",	"asterisk",	"plus",
	"comma",	"hyphen",	"period",	"slash",
	#  48
	"zero",		"one",		"two",		"three",
	"four",		"five",		"six",		"seven",
	"eight",	"nine",		"colon",	"semicolon",
	"less",		"equal",	"greater",	"question",
	#  64
	"at",		"A",		"B",		"C",
	"D",		"E",		"F",		"G",
	"H",		"I",		"J",		"K",
	"L",		"M",		"N",		"O",
	#  80
	"P",		"Q",		"R",		"S",
	"T",		"U",		"V",		"W",
	"X",		"Y",		"Z",		"bracketleft",
	"backslash",	"bracketright",	"asciicircum",	"underscore",
	#  96
	"grave",	"a",		"b",		"c",
	"d",		"e",		"f",		"g",
	"h",		"i",		"j",		"k",
	"l",		"m",		"n",		"o",
	# 112
	"p",		"q",		"r",		"s",
	"t",		"u",		"v",		"w",
	"x",		"y",		"z",		"braceleft",
	"bar",		"braceright",	"asciitilde",	NULL,
	# 128
	"Euro",		NULL,		"quotesinglbase","florin",
	"quotedblbase", "ellipsis",	"dagger",	"daggerdbl",
	"circumflex",	"perthousand",	"Scaron",	"guilsinglleft",
	"OE",		NULL,		"Zcaron",	NULL,
	# 144
	NULL,		"quoteleft",	"quoteright",	"quotedblleft",
	"quotedblright","bullet",	"endash",	"emdash",
	"tilde",	"trademark",	"scaron",	"guilsinglright",
	"oe",		NULL,		"zcaron",	"Ydieresis",
	# 160
	NULL,		"exclamdown",	"cent",		"sterling",
	"currency",	"yen",		"brokenbar",	"section",
	"dieresis",	"copyright",	"ordfeminine",	"guillemotleft",
	# 173 is also "hyphen" but we leave this NULL to avoid duplicate names
	"logicalnot",	NULL,		"registered",	"macron",
	# 176
	"degree",	"plusminus",	"twosuperior",	"threesuperior",
	"acute",	"mu",		"paragraph",	"periodcentered",
	"cedilla",	"onesuperior",	"ordmasculine",	"guillemotright",
	"onequarter",	"onehalf",	"threequarters","questiondown",
	# 192
	"Agrave",	"Aacute",	"Acircumflex",	"Atilde",
	"Adieresis",	"Aring",	"AE",		"Ccedilla",
	"Egrave",	"Eacute",	"Ecircumflex",	"Edieresis",
	"Igrave",	"Iacute",	"Icircumflex",	"Idieresis",
	# 208
	"Eth",		"Ntilde",	"Ograve",	"Oacute",
	"Ocircumflex",	"Otilde",	"Odieresis",	"multiply",
	"Oslash",	"Ugrave",	"Uacute",	"Ucircumflex",
	"Udieresis",	"Yacute",	"Thorn",	"germandbls",
	# 224
	"agrave",	"aacute",	"acircumflex",	"atilde",
	"adieresis",	"aring",	"ae",		"ccedilla",
	"egrave",	"eacute",	"ecircumflex",	"edieresis",
	"igrave",	"iacute",	"icircumflex",	"idieresis",
	# 240
	"eth",		"ntilde",	"ograve",	"oacute",
	"ocircumflex",	"otilde",	"odieresis",	"divide",
	"oslash",	"ugrave",	"uacute",	"ucircumflex",
	"udieresis",	"yacute",	"thorn",	"ydieresis"
);

sub print_offsets {
    $s = qq();
    for $sym (@_) {
        if (! ($sym eq NULL)) {
	    $ss = qq( $hash{$sym}/*$sym*/,);
	} else {
	    $ss = qq( 0,);
	}
	if (length($s) + length($ss) > 78) {
	    print qq( $s\n);
	    $s = "";
	}
	$s .= $ss;
    }
    print qq( $s\n);
}

@combined = (@ps_standard_encoding, @winansi_encoding);
print "static const char glyph_name_symbol[] = {\n";
%hash = ();
$s = qq( "\\0");
$offset = 1;
for $sym (@combined) {
    if (! ($sym eq NULL)) {
        if (! exists $hash{$sym}) {
	    $hash{$sym} = $offset;
	    $offset += length($sym) + 1;
	    $ss = qq( "$sym\\0");
	    if (length($s) + length($ss) > 78) {
	        print qq( $s\n);
	        $s = "";
	    }
	    $s .= $ss;
	}
    }
}
print qq( $s\n);
print "};\n\n";

print "static const int16_t ps_standard_encoding_offset[256] = {\n";
print_offsets(@ps_standard_encoding);
print "};\n";

print "static const int16_t winansi_encoding_offset[256] = {\n";
print_offsets(@winansi_encoding);
print "};\n";

exit;
#endif

/* FlashVM C89: byte initializer preserves the generated offsets exactly. */
static const char glyph_name_symbol[] = {
    0, 115, 112, 97, 99, 101, 0, 101, 120, 99, 108, 97, 109, 0, 113, 117, 111, 116, 101, 100,
    98, 108, 0, 110, 117, 109, 98, 101, 114, 115, 105, 103, 110, 0, 100, 111, 108, 108, 97, 114,
    0, 112, 101, 114, 99, 101, 110, 116, 0, 97, 109, 112, 101, 114, 115, 97, 110, 100, 0, 113,
    117, 111, 116, 101, 114, 105, 103, 104, 116, 0, 112, 97, 114, 101, 110, 108, 101, 102, 116, 0,
    112, 97, 114, 101, 110, 114, 105, 103, 104, 116, 0, 97, 115, 116, 101, 114, 105, 115, 107, 0,
    112, 108, 117, 115, 0, 99, 111, 109, 109, 97, 0, 104, 121, 112, 104, 101, 110, 0, 112, 101,
    114, 105, 111, 100, 0, 115, 108, 97, 115, 104, 0, 122, 101, 114, 111, 0, 111, 110, 101, 0,
    116, 119, 111, 0, 116, 104, 114, 101, 101, 0, 102, 111, 117, 114, 0, 102, 105, 118, 101, 0,
    115, 105, 120, 0, 115, 101, 118, 101, 110, 0, 101, 105, 103, 104, 116, 0, 110, 105, 110, 101,
    0, 99, 111, 108, 111, 110, 0, 115, 101, 109, 105, 99, 111, 108, 111, 110, 0, 108, 101, 115,
    115, 0, 101, 113, 117, 97, 108, 0, 103, 114, 101, 97, 116, 101, 114, 0, 113, 117, 101, 115,
    116, 105, 111, 110, 0, 97, 116, 0, 65, 0, 66, 0, 67, 0, 68, 0, 69, 0, 70, 0,
    71, 0, 72, 0, 73, 0, 74, 0, 75, 0, 76, 0, 77, 0, 78, 0, 79, 0, 80, 0,
    81, 0, 82, 0, 83, 0, 84, 0, 85, 0, 86, 0, 87, 0, 88, 0, 89, 0, 90, 0,
    98, 114, 97, 99, 107, 101, 116, 108, 101, 102, 116, 0, 98, 97, 99, 107, 115, 108, 97, 115,
    104, 0, 98, 114, 97, 99, 107, 101, 116, 114, 105, 103, 104, 116, 0, 97, 115, 99, 105, 105,
    99, 105, 114, 99, 117, 109, 0, 117, 110, 100, 101, 114, 115, 99, 111, 114, 101, 0, 113, 117,
    111, 116, 101, 108, 101, 102, 116, 0, 97, 0, 98, 0, 99, 0, 100, 0, 101, 0, 102, 0,
    103, 0, 104, 0, 105, 0, 106, 0, 107, 0, 108, 0, 109, 0, 110, 0, 111, 0, 112, 0,
    113, 0, 114, 0, 115, 0, 116, 0, 117, 0, 118, 0, 119, 0, 120, 0, 121, 0, 122, 0,
    98, 114, 97, 99, 101, 108, 101, 102, 116, 0, 98, 97, 114, 0, 98, 114, 97, 99, 101, 114,
    105, 103, 104, 116, 0, 97, 115, 99, 105, 105, 116, 105, 108, 100, 101, 0, 101, 120, 99, 108,
    97, 109, 100, 111, 119, 110, 0, 99, 101, 110, 116, 0, 115, 116, 101, 114, 108, 105, 110, 103,
    0, 102, 114, 97, 99, 116, 105, 111, 110, 0, 121, 101, 110, 0, 102, 108, 111, 114, 105, 110,
    0, 115, 101, 99, 116, 105, 111, 110, 0, 99, 117, 114, 114, 101, 110, 99, 121, 0, 113, 117,
    111, 116, 101, 115, 105, 110, 103, 108, 101, 0, 113, 117, 111, 116, 101, 100, 98, 108, 108, 101,
    102, 116, 0, 103, 117, 105, 108, 108, 101, 109, 111, 116, 108, 101, 102, 116, 0, 103, 117, 105,
    108, 115, 105, 110, 103, 108, 108, 101, 102, 116, 0, 103, 117, 105, 108, 115, 105, 110, 103, 108,
    114, 105, 103, 104, 116, 0, 102, 105, 0, 102, 108, 0, 101, 110, 100, 97, 115, 104, 0, 100,
    97, 103, 103, 101, 114, 0, 100, 97, 103, 103, 101, 114, 100, 98, 108, 0, 112, 101, 114, 105,
    111, 100, 99, 101, 110, 116, 101, 114, 101, 100, 0, 112, 97, 114, 97, 103, 114, 97, 112, 104,
    0, 98, 117, 108, 108, 101, 116, 0, 113, 117, 111, 116, 101, 115, 105, 110, 103, 108, 98, 97,
    115, 101, 0, 113, 117, 111, 116, 101, 100, 98, 108, 98, 97, 115, 101, 0, 113, 117, 111, 116,
    101, 100, 98, 108, 114, 105, 103, 104, 116, 0, 103, 117, 105, 108, 108, 101, 109, 111, 116, 114,
    105, 103, 104, 116, 0, 101, 108, 108, 105, 112, 115, 105, 115, 0, 112, 101, 114, 116, 104, 111,
    117, 115, 97, 110, 100, 0, 113, 117, 101, 115, 116, 105, 111, 110, 100, 111, 119, 110, 0, 103,
    114, 97, 118, 101, 0, 97, 99, 117, 116, 101, 0, 99, 105, 114, 99, 117, 109, 102, 108, 101,
    120, 0, 116, 105, 108, 100, 101, 0, 109, 97, 99, 114, 111, 110, 0, 98, 114, 101, 118, 101,
    0, 100, 111, 116, 97, 99, 99, 101, 110, 116, 0, 100, 105, 101, 114, 101, 115, 105, 115, 0,
    114, 105, 110, 103, 0, 99, 101, 100, 105, 108, 108, 97, 0, 104, 117, 110, 103, 97, 114, 117,
    109, 108, 97, 117, 116, 0, 111, 103, 111, 110, 101, 107, 0, 99, 97, 114, 111, 110, 0, 101,
    109, 100, 97, 115, 104, 0, 65, 69, 0, 111, 114, 100, 102, 101, 109, 105, 110, 105, 110, 101,
    0, 76, 115, 108, 97, 115, 104, 0, 79, 115, 108, 97, 115, 104, 0, 79, 69, 0, 111, 114,
    100, 109, 97, 115, 99, 117, 108, 105, 110, 101, 0, 97, 101, 0, 100, 111, 116, 108, 101, 115,
    115, 105, 0, 108, 115, 108, 97, 115, 104, 0, 111, 115, 108, 97, 115, 104, 0, 111, 101, 0,
    103, 101, 114, 109, 97, 110, 100, 98, 108, 115, 0, 69, 117, 114, 111, 0, 83, 99, 97, 114,
    111, 110, 0, 90, 99, 97, 114, 111, 110, 0, 116, 114, 97, 100, 101, 109, 97, 114, 107, 0,
    115, 99, 97, 114, 111, 110, 0, 122, 99, 97, 114, 111, 110, 0, 89, 100, 105, 101, 114, 101,
    115, 105, 115, 0, 98, 114, 111, 107, 101, 110, 98, 97, 114, 0, 99, 111, 112, 121, 114, 105,
    103, 104, 116, 0, 108, 111, 103, 105, 99, 97, 108, 110, 111, 116, 0, 114, 101, 103, 105, 115,
    116, 101, 114, 101, 100, 0, 100, 101, 103, 114, 101, 101, 0, 112, 108, 117, 115, 109, 105, 110,
    117, 115, 0, 116, 119, 111, 115, 117, 112, 101, 114, 105, 111, 114, 0, 116, 104, 114, 101, 101,
    115, 117, 112, 101, 114, 105, 111, 114, 0, 109, 117, 0, 111, 110, 101, 115, 117, 112, 101, 114,
    105, 111, 114, 0, 111, 110, 101, 113, 117, 97, 114, 116, 101, 114, 0, 111, 110, 101, 104, 97,
    108, 102, 0, 116, 104, 114, 101, 101, 113, 117, 97, 114, 116, 101, 114, 115, 0, 65, 103, 114,
    97, 118, 101, 0, 65, 97, 99, 117, 116, 101, 0, 65, 99, 105, 114, 99, 117, 109, 102, 108,
    101, 120, 0, 65, 116, 105, 108, 100, 101, 0, 65, 100, 105, 101, 114, 101, 115, 105, 115, 0,
    65, 114, 105, 110, 103, 0, 67, 99, 101, 100, 105, 108, 108, 97, 0, 69, 103, 114, 97, 118,
    101, 0, 69, 97, 99, 117, 116, 101, 0, 69, 99, 105, 114, 99, 117, 109, 102, 108, 101, 120,
    0, 69, 100, 105, 101, 114, 101, 115, 105, 115, 0, 73, 103, 114, 97, 118, 101, 0, 73, 97,
    99, 117, 116, 101, 0, 73, 99, 105, 114, 99, 117, 109, 102, 108, 101, 120, 0, 73, 100, 105,
    101, 114, 101, 115, 105, 115, 0, 69, 116, 104, 0, 78, 116, 105, 108, 100, 101, 0, 79, 103,
    114, 97, 118, 101, 0, 79, 97, 99, 117, 116, 101, 0, 79, 99, 105, 114, 99, 117, 109, 102,
    108, 101, 120, 0, 79, 116, 105, 108, 100, 101, 0, 79, 100, 105, 101, 114, 101, 115, 105, 115,
    0, 109, 117, 108, 116, 105, 112, 108, 121, 0, 85, 103, 114, 97, 118, 101, 0, 85, 97, 99,
    117, 116, 101, 0, 85, 99, 105, 114, 99, 117, 109, 102, 108, 101, 120, 0, 85, 100, 105, 101,
    114, 101, 115, 105, 115, 0, 89, 97, 99, 117, 116, 101, 0, 84, 104, 111, 114, 110, 0, 97,
    103, 114, 97, 118, 101, 0, 97, 97, 99, 117, 116, 101, 0, 97, 99, 105, 114, 99, 117, 109,
    102, 108, 101, 120, 0, 97, 116, 105, 108, 100, 101, 0, 97, 100, 105, 101, 114, 101, 115, 105,
    115, 0, 97, 114, 105, 110, 103, 0, 99, 99, 101, 100, 105, 108, 108, 97, 0, 101, 103, 114,
    97, 118, 101, 0, 101, 97, 99, 117, 116, 101, 0, 101, 99, 105, 114, 99, 117, 109, 102, 108,
    101, 120, 0, 101, 100, 105, 101, 114, 101, 115, 105, 115, 0, 105, 103, 114, 97, 118, 101, 0,
    105, 97, 99, 117, 116, 101, 0, 105, 99, 105, 114, 99, 117, 109, 102, 108, 101, 120, 0, 105,
    100, 105, 101, 114, 101, 115, 105, 115, 0, 101, 116, 104, 0, 110, 116, 105, 108, 100, 101, 0,
    111, 103, 114, 97, 118, 101, 0, 111, 97, 99, 117, 116, 101, 0, 111, 99, 105, 114, 99, 117,
    109, 102, 108, 101, 120, 0, 111, 116, 105, 108, 100, 101, 0, 111, 100, 105, 101, 114, 101, 115,
    105, 115, 0, 100, 105, 118, 105, 100, 101, 0, 117, 103, 114, 97, 118, 101, 0, 117, 97, 99,
    117, 116, 101, 0, 117, 99, 105, 114, 99, 117, 109, 102, 108, 101, 120, 0, 117, 100, 105, 101,
    114, 101, 115, 105, 115, 0, 121, 97, 99, 117, 116, 101, 0, 116, 104, 111, 114, 110, 0, 121,
    100, 105, 101, 114, 101, 115, 105, 115, 0, 0,
};

static const int16_t ps_standard_encoding_offset[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1/*space*/, 7/*exclam*/, 14/*quotedbl*/, 23/*numbersign*/,
  34/*dollar*/, 41/*percent*/, 49/*ampersand*/, 59/*quoteright*/,
  70/*parenleft*/, 80/*parenright*/, 91/*asterisk*/, 100/*plus*/, 105/*comma*/,
  111/*hyphen*/, 118/*period*/, 125/*slash*/, 131/*zero*/, 136/*one*/,
  140/*two*/, 144/*three*/, 150/*four*/, 155/*five*/, 160/*six*/, 164/*seven*/,
  170/*eight*/, 176/*nine*/, 181/*colon*/, 187/*semicolon*/, 197/*less*/,
  202/*equal*/, 208/*greater*/, 216/*question*/, 225/*at*/, 228/*A*/, 230/*B*/,
  232/*C*/, 234/*D*/, 236/*E*/, 238/*F*/, 240/*G*/, 242/*H*/, 244/*I*/,
  246/*J*/, 248/*K*/, 250/*L*/, 252/*M*/, 254/*N*/, 256/*O*/, 258/*P*/,
  260/*Q*/, 262/*R*/, 264/*S*/, 266/*T*/, 268/*U*/, 270/*V*/, 272/*W*/,
  274/*X*/, 276/*Y*/, 278/*Z*/, 280/*bracketleft*/, 292/*backslash*/,
  302/*bracketright*/, 315/*asciicircum*/, 327/*underscore*/, 338/*quoteleft*/,
  348/*a*/, 350/*b*/, 352/*c*/, 354/*d*/, 356/*e*/, 358/*f*/, 360/*g*/,
  362/*h*/, 364/*i*/, 366/*j*/, 368/*k*/, 370/*l*/, 372/*m*/, 374/*n*/,
  376/*o*/, 378/*p*/, 380/*q*/, 382/*r*/, 384/*s*/, 386/*t*/, 388/*u*/,
  390/*v*/, 392/*w*/, 394/*x*/, 396/*y*/, 398/*z*/, 400/*braceleft*/,
  410/*bar*/, 414/*braceright*/, 425/*asciitilde*/, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  436/*exclamdown*/, 447/*cent*/, 452/*sterling*/, 461/*fraction*/, 470/*yen*/,
  474/*florin*/, 481/*section*/, 489/*currency*/, 498/*quotesingle*/,
  510/*quotedblleft*/, 523/*guillemotleft*/, 537/*guilsinglleft*/,
  551/*guilsinglright*/, 566/*fi*/, 569/*fl*/, 0, 572/*endash*/, 579/*dagger*/,
  586/*daggerdbl*/, 596/*periodcentered*/, 0, 611/*paragraph*/, 621/*bullet*/,
  628/*quotesinglbase*/, 643/*quotedblbase*/, 656/*quotedblright*/,
  670/*guillemotright*/, 685/*ellipsis*/, 694/*perthousand*/, 0,
  706/*questiondown*/, 0, 719/*grave*/, 725/*acute*/, 731/*circumflex*/,
  742/*tilde*/, 748/*macron*/, 755/*breve*/, 761/*dotaccent*/, 771/*dieresis*/,
  0, 780/*ring*/, 785/*cedilla*/, 0, 793/*hungarumlaut*/, 806/*ogonek*/,
  813/*caron*/, 819/*emdash*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  826/*AE*/, 0, 829/*ordfeminine*/, 0, 0, 0, 0, 841/*Lslash*/, 848/*Oslash*/,
  855/*OE*/, 858/*ordmasculine*/, 0, 0, 0, 0, 0, 871/*ae*/, 0, 0, 0,
  874/*dotlessi*/, 0, 0, 883/*lslash*/, 890/*oslash*/, 897/*oe*/,
  900/*germandbls*/, 0, 0, 0, 0,
};

static const int16_t winansi_encoding_offset[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1/*space*/, 7/*exclam*/, 14/*quotedbl*/, 23/*numbersign*/,
  34/*dollar*/, 41/*percent*/, 49/*ampersand*/, 498/*quotesingle*/,
  70/*parenleft*/, 80/*parenright*/, 91/*asterisk*/, 100/*plus*/, 105/*comma*/,
  111/*hyphen*/, 118/*period*/, 125/*slash*/, 131/*zero*/, 136/*one*/,
  140/*two*/, 144/*three*/, 150/*four*/, 155/*five*/, 160/*six*/, 164/*seven*/,
  170/*eight*/, 176/*nine*/, 181/*colon*/, 187/*semicolon*/, 197/*less*/,
  202/*equal*/, 208/*greater*/, 216/*question*/, 225/*at*/, 228/*A*/, 230/*B*/,
  232/*C*/, 234/*D*/, 236/*E*/, 238/*F*/, 240/*G*/, 242/*H*/, 244/*I*/,
  246/*J*/, 248/*K*/, 250/*L*/, 252/*M*/, 254/*N*/, 256/*O*/, 258/*P*/,
  260/*Q*/, 262/*R*/, 264/*S*/, 266/*T*/, 268/*U*/, 270/*V*/, 272/*W*/,
  274/*X*/, 276/*Y*/, 278/*Z*/, 280/*bracketleft*/, 292/*backslash*/,
  302/*bracketright*/, 315/*asciicircum*/, 327/*underscore*/, 719/*grave*/,
  348/*a*/, 350/*b*/, 352/*c*/, 354/*d*/, 356/*e*/, 358/*f*/, 360/*g*/,
  362/*h*/, 364/*i*/, 366/*j*/, 368/*k*/, 370/*l*/, 372/*m*/, 374/*n*/,
  376/*o*/, 378/*p*/, 380/*q*/, 382/*r*/, 384/*s*/, 386/*t*/, 388/*u*/,
  390/*v*/, 392/*w*/, 394/*x*/, 396/*y*/, 398/*z*/, 400/*braceleft*/,
  410/*bar*/, 414/*braceright*/, 425/*asciitilde*/, 0, 911/*Euro*/, 0,
  628/*quotesinglbase*/, 474/*florin*/, 643/*quotedblbase*/, 685/*ellipsis*/,
  579/*dagger*/, 586/*daggerdbl*/, 731/*circumflex*/, 694/*perthousand*/,
  916/*Scaron*/, 537/*guilsinglleft*/, 855/*OE*/, 0, 923/*Zcaron*/, 0, 0,
  338/*quoteleft*/, 59/*quoteright*/, 510/*quotedblleft*/,
  656/*quotedblright*/, 621/*bullet*/, 572/*endash*/, 819/*emdash*/,
  742/*tilde*/, 930/*trademark*/, 940/*scaron*/, 551/*guilsinglright*/,
  897/*oe*/, 0, 947/*zcaron*/, 954/*Ydieresis*/, 0, 436/*exclamdown*/,
  447/*cent*/, 452/*sterling*/, 489/*currency*/, 470/*yen*/, 964/*brokenbar*/,
  481/*section*/, 771/*dieresis*/, 974/*copyright*/, 829/*ordfeminine*/,
  523/*guillemotleft*/, 984/*logicalnot*/, 0, 995/*registered*/, 748/*macron*/,
  1006/*degree*/, 1013/*plusminus*/, 1023/*twosuperior*/,
  1035/*threesuperior*/, 725/*acute*/, 1049/*mu*/, 611/*paragraph*/,
  596/*periodcentered*/, 785/*cedilla*/, 1052/*onesuperior*/,
  858/*ordmasculine*/, 670/*guillemotright*/, 1064/*onequarter*/,
  1075/*onehalf*/, 1083/*threequarters*/, 706/*questiondown*/, 1097/*Agrave*/,
  1104/*Aacute*/, 1111/*Acircumflex*/, 1123/*Atilde*/, 1130/*Adieresis*/,
  1140/*Aring*/, 826/*AE*/, 1146/*Ccedilla*/, 1155/*Egrave*/, 1162/*Eacute*/,
  1169/*Ecircumflex*/, 1181/*Edieresis*/, 1191/*Igrave*/, 1198/*Iacute*/,
  1205/*Icircumflex*/, 1217/*Idieresis*/, 1227/*Eth*/, 1231/*Ntilde*/,
  1238/*Ograve*/, 1245/*Oacute*/, 1252/*Ocircumflex*/, 1264/*Otilde*/,
  1271/*Odieresis*/, 1281/*multiply*/, 848/*Oslash*/, 1290/*Ugrave*/,
  1297/*Uacute*/, 1304/*Ucircumflex*/, 1316/*Udieresis*/, 1326/*Yacute*/,
  1333/*Thorn*/, 900/*germandbls*/, 1339/*agrave*/, 1346/*aacute*/,
  1353/*acircumflex*/, 1365/*atilde*/, 1372/*adieresis*/, 1382/*aring*/,
  871/*ae*/, 1388/*ccedilla*/, 1397/*egrave*/, 1404/*eacute*/,
  1411/*ecircumflex*/, 1423/*edieresis*/, 1433/*igrave*/, 1440/*iacute*/,
  1447/*icircumflex*/, 1459/*idieresis*/, 1469/*eth*/, 1473/*ntilde*/,
  1480/*ograve*/, 1487/*oacute*/, 1494/*ocircumflex*/, 1506/*otilde*/,
  1513/*odieresis*/, 1523/*divide*/, 890/*oslash*/, 1530/*ugrave*/,
  1537/*uacute*/, 1544/*ucircumflex*/, 1556/*udieresis*/, 1566/*yacute*/,
  1573/*thorn*/, 1579/*ydieresis*/,
};

const char *
_cairo_ps_standard_encoding_to_glyphname (int glyph)
{
    if (ps_standard_encoding_offset[glyph])
	return glyph_name_symbol + ps_standard_encoding_offset[glyph];
    else
	return NULL;
}

const char *
_cairo_winansi_to_glyphname (int glyph)
{
    if (winansi_encoding_offset[glyph])
	return glyph_name_symbol + winansi_encoding_offset[glyph];
    else
	return NULL;
}

#endif /* CAIRO_HAS_FONT_SUBSET */
