/* Copyright (c) 2026 bhgv
 * EN: Internal display-list queries for a standard TextField.
 * RU: Внутренние запросы списка отображения для штатного TextField.
 */
#ifndef FVM_TEXT_DISPLAY_HPP
#define FVM_TEXT_DISPLAY_HPP
#include "flashvm/standard_text.h"


/* EN: Propose a Unicode scalar insertion point in field-local pixels, or -1 outside.
 * RU: Вернуть позицию вставки в локальных пикселях поля либо -1 за его пределами. */
FVM_Result fvm_textfield_scalar_at(FVM_Context *, FVM_Object *, double x, double y, long &index);


/* EN: Export detached preview paths and their two RGB colors. Empty previews return null paths.
 * RU: Выдать отдельные контуры предпросмотра и два цвета RGB. Пустой результат содержит null. */
FVM_Result fvm_textfield_preview_paths(FVM_Context *, FVM_Object *, FVM_Path **selection, FVM_Path **caret,
    double &selection_rgb, double &caret_rgb);
#endif
