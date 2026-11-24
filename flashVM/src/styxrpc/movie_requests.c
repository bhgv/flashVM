/* Copyright (c) 2026 bhgv
 * EN: Project AVM1 MovieClip and URL operations onto the deferred Styx movie loader.
 * RU: Подключение операций MovieClip и URL AVM1 к отложенной загрузке Styx.
 */
#include "movie_load.h"
#include "flashvm/avm1.h"
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include <string.h>

#define MOVIE_REQUEST_FIELDS 3U /* URL, target name, and AVM1 URL flags. */
#define MOVIE_REQUEST_URL 0U    /* Exact counted URL passed by the VM. */
#define MOVIE_REQUEST_TARGET 1U /* Counted target path or an already selected clip. */
#define MOVIE_REQUEST_FLAGS 2U  /* AVM1 request flags carried as an exact Number. */
#define MOVIE_METHOD_SLOT 1U    /* Optional retrieval method in the MovieClip method form. */
#define MOVIE_GLOBAL_SLOT 2U    /* Optional retrieval method after a global target or level. */


/* EN: Match a counted script string against one built-in word.
 * RU: Сравнить строку сценария с длиной с одним именем встроенной операции. */
static int named(const FVM_Value *value, const char *word)
{
    size_t size = strlen(word);
    return value->type == FVM_VALUE_STRING && fvm_value_string_size(value) == size &&
        !memcmp(value->string, word, size);
}


/* [GC:] [ASINC] EN: Resolve the active AVM1 origin and the exact destination before the frame returns.
 * RU: Разрешить источник AVM1 и точное назначение до выхода из кадра. */
static FVM_Result request(FVM_StyxRpcMovies *movies, const FVM_Value *url, const FVM_Value *target)
{
    FVM_Context *context = movies->context;
    FVM_AVM1TargetInfo info;
    FVM_Result status;
    FVM_Value selected;
    info.struct_size = sizeof(info);
    status = fvm_avm1_get_target_info(context, &info);
    if (status != FVM_OK || !info.active || !info.base_clip)
    {
        return fvm_styxrpc_movie_reject(movies, FVM_NOT_FOUND);
    }
    if (target->type == FVM_VALUE_OBJECT)
    {
        selected = *target;
    }
    else if (named(target, "_level0") || !fvm_value_string_size(target))
    {
        fvm_value_undefined(&selected);
        selected.type = FVM_VALUE_OBJECT;
        selected.object = named(target, "_level0") ? movies->root : info.target_clip;
    }
    else
    {
        status = fvm_exec_scope_target(context, NULL, info.target_clip, info.base_clip, target, &selected);
        if (status != FVM_OK)
        {
            return fvm_styxrpc_movie_reject(movies, status);
        }
    }
    if (selected.type != FVM_VALUE_OBJECT || !selected.object)
    {
        return fvm_styxrpc_movie_reject(movies, FVM_NOT_FOUND);
    }
    return fvm_styxrpc_movie_queue(movies, info.base_clip, selected.object, url->string, fvm_value_string_size(url));
}


/* [ASINC] EN: Admit only movie-target operations and the original level-zero legacy spelling.
 * RU: Разрешить загрузку в клип и старую форму для нулевого уровня. */
static FVM_Result request_url(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    FVM_StyxRpcMovies *movies = state;
    unsigned int flags;
    (void)context;
    (void)receiver;
    (void)result;
    if (argc != MOVIE_REQUEST_FIELDS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    flags = (unsigned int)args[MOVIE_REQUEST_FLAGS].number;
    if (flags != FVM_AVM1_URL_LOAD_TARGET &&
        flags != (FVM_AVM1_URL_LOAD_TARGET | FVM_AVM1_URL_METHOD_GET) &&
        !(flags == FVM_AVM1_URL_METHOD_NONE && named(args + MOVIE_REQUEST_TARGET, "_level0")))
    {
        return fvm_styxrpc_movie_reject(movies, FVM_UNSUPPORTED);
    }
    return request(movies, args + MOVIE_REQUEST_URL, args + MOVIE_REQUEST_TARGET);
}


/* [ASINC] EN: Coerce the URL and forward to the normal AVM1 service or keep an object target.
 * RU: Преобразовать URL и вызвать службу AVM1 либо сохранить объект назначения. */
static FVM_Result request_value(FVM_Context *context, FVM_StyxRpcMovies *movies,
    const FVM_Value *url, const FVM_Value *target)
{
    FVM_Value text;
    FVM_Result status;
    if (target->type != FVM_VALUE_OBJECT)
    {
        return fvm_exec_url(context, url, target, FVM_AVM1_URL_LOAD_TARGET);
    }
    status = fvm_exec_to_string(context, url, &text);
    return status == FVM_OK ? request(movies, &text, target) : status;
}


/* [ASINC] EN: Dispatch a global or inherited MovieClip loadMovie call.
 * RU: Обработать глобальный либо унаследованный вызов MovieClip.loadMovie. */
static FVM_Result load(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    FVM_StyxRpcMovies *movies = state;
    FVM_Value target;
    int method_form = receiver != NULL;
    unsigned int index;
    if (result)
    {
        fvm_value_undefined(result);
    }
    if (method_form && argc > MOVIE_METHOD_SLOT &&
        (argc > MOVIE_GLOBAL_SLOT || args[MOVIE_REQUEST_TARGET].type == FVM_VALUE_OBJECT ||
         (args[MOVIE_REQUEST_TARGET].type == FVM_VALUE_STRING && !named(args + MOVIE_REQUEST_TARGET, "GET") &&
          !named(args + MOVIE_REQUEST_TARGET, "POST"))))
    {
        method_form = 0;
    }
    index = method_form ? MOVIE_METHOD_SLOT : MOVIE_GLOBAL_SLOT;
    if (argc < index || argc > index + 1)
    {
        return fvm_styxrpc_movie_reject(movies, FVM_INVALID_ARGUMENT);
    }
    if (argc > index && args[index].type != FVM_VALUE_UNDEFINED && !named(args + index, "GET"))
    {
        return fvm_styxrpc_movie_reject(movies, FVM_UNSUPPORTED);
    }
    if (method_form)
    {
        fvm_value_undefined(&target);
        target.type = FVM_VALUE_OBJECT;
        target.object = receiver;
    }
    else
    {
        target = args[MOVIE_REQUEST_TARGET];
    }
    return request_value(context, movies, args, &target);
}


/* [ASINC] EN: Preserve the global argument shape even when the runtime supplies a receiver.
 * RU: Сохранять глобальную форму аргументов при передаче получателя средой выполнения. */
static FVM_Result load_global(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    (void)receiver;
    return load(context, NULL, argc, args, result, state);
}


/* [ASINC] EN: Recognize the existing root level; separate numbered display roots are unsupported.
 * RU: Распознать существующий корень; отдельные нумерованные сцены не поддерживаются. */
static FVM_Result load_level(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    FVM_StyxRpcMovies *movies = state;
    FVM_Value level, target;
    FVM_Result status;
    (void)receiver;
    if (result)
    {
        fvm_value_undefined(result);
    }
    if (argc < MOVIE_GLOBAL_SLOT || argc > MOVIE_GLOBAL_SLOT + 1)
    {
        return fvm_styxrpc_movie_reject(movies, FVM_INVALID_ARGUMENT);
    }
    status = fvm_exec_to_number(context, 0, args + MOVIE_REQUEST_TARGET, &level);
    if (status != FVM_OK)
    {
        return status;
    }
    if (level.number != 0 || (argc > MOVIE_GLOBAL_SLOT &&
        args[MOVIE_GLOBAL_SLOT].type != FVM_VALUE_UNDEFINED && !named(args + MOVIE_GLOBAL_SLOT, "GET")))
    {
        return fvm_styxrpc_movie_reject(movies, FVM_UNSUPPORTED);
    }
    fvm_value_undefined(&target);
    target.type = FVM_VALUE_OBJECT;
    target.object = movies->root;
    return request_value(context, movies, args, &target);
}


/* [GC:] EN/RU: See movie_load.h. / См. movie_load.h. */
FVM_Result fvm_styxrpc_movies_bind(FVM_StyxRpcMovies *movies)
{
    FVM_Context *context = movies->context;
    FVM_Value constructor, prototype;
    FVM_Result status = fvm_get_global(context, "MovieClip", &constructor);
    if (status != FVM_OK || constructor.type != FVM_VALUE_OBJECT)
    {
        return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
    }
    status = fvm_object_get(context, constructor.object, "prototype", &prototype);
    if (status != FVM_OK || prototype.type != FVM_VALUE_OBJECT)
    {
        return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
    }
    status = fvm_movieclip_set_source(context, movies->root, "/", 1);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, prototype.object, "loadMovie", load, movies, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_register_function(context, "loadMovie", load_global, movies);
    }
    if (status == FVM_OK)
    {
        status = fvm_register_function(context, "loadMovieNum", load_level, movies);
    }
    if (status == FVM_OK)
    {
        status = fvm_avm1_set_url_rooted(context, request_url, movies, NULL);
    }
    return status;
}
