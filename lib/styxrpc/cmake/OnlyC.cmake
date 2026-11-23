# Copyright (c) 2026 bhgv
# EN: Reject accidental C++ units/language overrides in exported C libraries.
# RU: Отвергнуть C++ units/language overrides в экспортируемых C-библиотеках.


# EN: Check each borrowed target name in ARGN; compile units must end in .c,
# headers/includes may accompany them. Reject CXX source/linker overrides at
# configure time. Examples and downstream consumers are deliberately not passed.
# RU: Проверить target names из ARGN; compile units должны быть .c, headers/includes
# разрешены рядом с ними. Отвергнуть CXX source/linker override при configure.
# Примеры и downstream consumers сюда намеренно не передаются.
function(require_c_libraries)
    foreach(target IN LISTS ARGN)
        get_target_property(linker "${target}" LINKER_LANGUAGE)
        if(linker AND NOT linker STREQUAL "C")
            message(FATAL_ERROR "C-only library ${target}: linker language ${linker}")
        endif()
        get_target_property(sources "${target}" SOURCES)
        foreach(source IN LISTS sources)
            if(NOT source MATCHES "\\.(c|h|inc)$")
                message(FATAL_ERROR "C-only library ${target}: source ${source}")
            endif()
            get_source_file_property(language "${source}" LANGUAGE)
            if(language AND NOT language STREQUAL "C")
                message(FATAL_ERROR "C-only library ${target}: source language ${language}")
            endif()
        endforeach()
        set_target_properties("${target}" PROPERTIES LINKER_LANGUAGE C)
    endforeach()
endfunction()
