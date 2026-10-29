/* Real SDL FIFO burst tests, independent of SWF frames or physical keyboards. */
#include "editor_text_events.h"
#include "flashvm/keyboard.h"
#include <stdio.h>
#include <string.h>


/* Push one complete counted native text record; source must fit SDL's buffer. */
static int push_text(unsigned window, const char *text)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_TEXTINPUT;
    event.text.windowID = window;
    memcpy(event.text.text, text, strlen(text) + 1);
    return SDL_PushEvent(&event) == 1;
}


/* Check UTF-8 batching, retained native key state and command/capacity barriers.
 * Own SDL/context/runtime; no render, sleep, frame advance or mocked document. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    SDL_TextInputEvent first;
    SDL_Event event;
    DVM_UIMessage message;
    FVM_KeyboardState keyboard;
    unsigned i;
    int ok = 0;
    if (SDL_Init(SDL_INIT_EVENTS) || fvm_config_init(&config) != FVM_OK ||
        fvm_runtime_create(&config, &runtime) != FVM_OK || fvm_context_create(runtime, &context) != FVM_OK)
    {
        goto done;
    }
    memset(&first, 0, sizeof(first));
    first.type = SDL_TEXTINPUT;
    first.windowID = 1;
    strcpy(first.text, "A");
    for (i = 0; i < 50; i++)
    {
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.windowID = 1;
        event.key.keysym.sym = SDLK_a;
        if (SDL_PushEvent(&event) != 1 || !push_text(1, "\xd1\x8f"))
        {
            goto done;
        }
        event.type = SDL_KEYUP;
        if (SDL_PushEvent(&event) != 1)
        {
            goto done;
        }
    }
    memset(&event, 0, sizeof(event));
    event.type = SDL_KEYDOWN;
    event.key.windowID = 1;
    event.key.keysym.sym = SDLK_s;
    event.key.keysym.mod = KMOD_CTRL;
    if (SDL_PushEvent(&event) != 1 || !push_text(1, "B") ||
        !dvm_editor_text_batch(context, &first, &message) || message.size != 101 || message.text[0] != 'A')
    {
        goto done;
    }
    for (i = 1; i < message.size; i += 2)
    {
        if (memcmp(message.text + i, "\xd1\x8f", 2))
        {
            goto done;
        }
    }
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.struct_size = sizeof(keyboard);
    if (fvm_input_get_keyboard(context, &keyboard) != FVM_OK || keyboard.down['A'] ||
        SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1 ||
        event.type != SDL_KEYDOWN || !(event.key.keysym.mod & KMOD_CTRL))
    {
        goto done;
    }
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    memset(first.text, 'x', sizeof(first.text) - 1);
    first.text[sizeof(first.text) - 1] = 0;
    for (i = 0; i < 40; i++)
    {
        if (!push_text(1, first.text))
        {
            goto done;
        }
    }
    if (!dvm_editor_text_batch(context, &first, &message) || message.size != 1023 ||
        SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1 || event.type != SDL_TEXTINPUT)
    {
        goto done;
    }
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    if (!push_text(2, "foreign") || !dvm_editor_text_batch(context, &first, &message) || message.size != 31 ||
        SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1 || event.text.windowID != 2)
    {
        goto done;
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "SDL text burst/state/order/capacity test failed: %s\n", SDL_GetError());
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    SDL_Quit();
    return ok ? 0 : 1;
}
