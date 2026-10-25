# Give the historical parser its missing initial sentinel stack slot.
# INPUT is a generated/pinned parser; OUTPUT is build-owned. Upstream bytes stay exact.
file(READ "${INPUT}" parser)
foreach(anchor "yys[YYMAXDEPTH]," "yyp = &yys[-1];" "yyp >= &yys[YYMAXDEPTH]")
  string(FIND "${parser}" "${anchor}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "Bootstrap parser stack boundary changed; review sentinel adaptation")
  endif()
endforeach()
string(REPLACE "yys[YYMAXDEPTH]," "yys[YYMAXDEPTH+1]," parser "${parser}")
string(REPLACE "yyp = &yys[-1];" "yyp = &yys[0];" parser "${parser}")
string(REPLACE "yyp >= &yys[YYMAXDEPTH]" "yyp >= &yys[YYMAXDEPTH+1]" parser "${parser}")
file(WRITE "${OUTPUT}" "${parser}")
