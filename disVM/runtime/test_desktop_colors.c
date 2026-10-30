/* Validate actual native colour-message admission independently of the Limbo producer. */
#include "desktop_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Keep every predicate active in Release; message identifies the failed boundary. */
static void require(int ok, const char *message)
{
    if (!ok)
    {
        fprintf(stderr, "FAIL desktop colours: %s\n", message);
        exit(EXIT_FAILURE);
    }
}


/* Initialize a copied descriptor; text is borrowed only during this helper. */
static void payload(DVM_UIMessage *message, unsigned long code, const char *text)
{
    memset(message, 0, sizeof(*message));
    message->kind = DVM_UI_VIEW;
    message->code = code;
    message->flags = 7;
    message->size = strlen(text);
    memcpy(message->text, text, message->size);
}


/* Exercise UTF-8 scalar counts, palette classes, staged versus paint-only replies,
 * stale tickets and source metadata bounds. No mock parser or UI policy is used. */
int main(void)
{
    DVM_UIMessage message;
    payload(&message, DVM_DESK_BEGIN, "00000007000000010000000000000064");
    message.first = 960;
    message.second = 640;
    require(dvm_desktop_message(&message) == 1, "begin");
    payload(&message,
            DVM_DESK_ITEM,
            "000000010000006000000040000001a00000011000000009ffffffff00000000ffffffffEditor\n\317\200\360\237\230\200");
    message.first = 1;
    require(dvm_desktop_message(&message) == 1, "Unicode editor item");
    payload(&message, DVM_DESK_COLORS, "24");
    message.first = 1;
    message.second = 1;
    require(dvm_desktop_message(&message) == 1, "staged scalar palette");
    message.size = 1;
    require(!dvm_desktop_message(&message), "byte/scalar extent mismatch");
    message.size = 2;
    message.text[1] = '6';
    require(!dvm_desktop_message(&message), "unknown class");
    message.text[1] = '4';
    message.flags = 6;
    require(!dvm_desktop_message(&message), "stale view ticket");
    message.flags = 7;
    message.second = -1;
    require(!dvm_desktop_message(&message), "negative revision");
    message.second = 1;
    message.first = 2;
    require(!dvm_desktop_message(&message), "absent pane");
    message.first = 1;
    message.size = 201;
    require(!dvm_desktop_message(&message), "palette quota before reading payload");
    payload(&message, DVM_DESK_END, "");
    require(dvm_desktop_message(&message) == 1, "complete scene");
    payload(&message, DVM_DESK_COLORS, "35");
    message.first = 1;
    message.second = 1;
    require(dvm_desktop_message(&message) == 2, "paint-only update has distinct acknowledgement");
    require(dvm_desktop_ticket() == 7, "paint does not advance geometry/input ticket");
    message.flags = 8;
    require(!dvm_desktop_message(&message) && dvm_desktop_ticket() == 7, "rejected update preserves committed ticket");
    dvm_desktop_destroy();
    puts("PASS native colour protocol: Unicode extent, classes, quotas, same-ticket paint");
    return 0;
}
