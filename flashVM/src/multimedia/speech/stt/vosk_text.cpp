/* OS: portable. Parse the exact one-string JSON shape emitted by the configured Vosk recognizer. */
#include "../../../core/unicode_constants.h" /* Reuse semantic UTF-16/UTF-8 representation constants. */
#include "flashvm/utf8.h"                    /* Validate and encode actual Unicode scalars. */
#include "linux_vosk.hpp"                    /* Share the private decoder contract independently of native SDKs. */
#include <new>                               /* Allocate returned text without exceptions or STL. */
#include <string.h>                          /* Inspect the bounded JSON source and expected key. */
#define FVM_SPEECH_JSON_HEX_DIGITS 4         /* JSON encodes one UTF-16 code unit with four hexadecimal digits. */
#define FVM_SPEECH_JSON_HEX_RADIX 16UL       /* Numeric base of one JSON Unicode escape. */
#define FVM_SPEECH_JSON_CONTROL_LIMIT 32U    /* Unescaped JSON string bytes below this bound are invalid. */


/* Skip only JSON's four defined whitespace characters. */
static void skip_space(const char *&cursor)                                         /* Preserve strict object framing. */
{                                                                                   /* Begin bounded whitespace traversal. */
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') /* Stop at the next actual JSON token or NUL. */
    {                                                                               /* Begin loop body: Stop at the next actual JSON token or NUL. */
        ++cursor;                                                                   /* Stop at the next actual JSON token or NUL. */
    } /* End loop body: Stop at the next actual JSON token or NUL. */
} /* Finish whitespace traversal. */


/* Read exactly one Unicode escape's UTF-16 code unit. */
static bool read_unit(const char *&cursor, unsigned long &unit) /* Consume no bytes past the source terminator. */
{                                                               /* Begin Unicode escape decoding. */
    unit = 0;                                                   /* Accumulate exactly four hexadecimal digits. */
    for (unsigned int i = 0; i < FVM_SPEECH_JSON_HEX_DIGITS;    /* Bound escape parsing independently of provider text. */
         ++i)                                                   /* Bound escape parsing independently of provider text. */
    {                                                           /* Begin one hexadecimal digit. */
        unsigned char digit = (unsigned char)*cursor;           /* Inspect the current byte before advancing. */
        unsigned int value;                                     /* Store its numeric nibble value. */
        if (digit >= '0' && digit <= '9')                       /* Decode a decimal hexadecimal digit. */
        {                                                       /* Begin conditional branch: Decode a decimal hexadecimal digit. */
            value = digit - '0';                                /* Decode a decimal hexadecimal digit. */
        } /* End conditional branch: Decode a decimal hexadecimal digit. */
        else if (digit >= 'a' && digit <= 'f') /* Decode a lowercase hexadecimal digit. */
        {                                      /* Begin conditional branch: Decode a lowercase hexadecimal digit. */
            value = digit - 'a' + 10U;         /* Decode a lowercase hexadecimal digit. */
        } /* End conditional branch: Decode a lowercase hexadecimal digit. */
        else if (digit >= 'A' && digit <= 'F') /* Decode an uppercase hexadecimal digit. */
        {                                      /* Begin conditional branch: Decode an uppercase hexadecimal digit. */
            value = digit - 'A' + 10U;         /* Decode an uppercase hexadecimal digit. */
        } /* End conditional branch: Decode an uppercase hexadecimal digit. */
        else              /* Reject truncation and non-hexadecimal escape syntax. */
        {                 /* Begin conditional branch: Reject truncation and non-hexadecimal escape syntax. */
            return false; /* Reject truncation and non-hexadecimal escape syntax. */
        } /* End conditional branch: Reject truncation and non-hexadecimal escape syntax. */
        unit = unit * FVM_SPEECH_JSON_HEX_RADIX + value; /* Shift and append one bounded nibble. */
        ++cursor;                                        /* Advance only after validating this digit. */
    } /* Finish the four-digit code unit. */
    return true; /* Report one completely decoded UTF-16 unit. */
} /* Finish Unicode escape decoding. */


/* Return owned plain UTF-8, never JSON markup, for the exact enabled result configuration. */
FVM_Result fvm_vosk_text(const char *json, bool final, char **out) /* Empty recognized text returns NOT_FOUND. */
{                                                                  /* Begin bounded transcript extraction. */
    if (!out)                                                      /* Require an owned-text output slot. */
    {                                                              /* Begin conditional branch: Require an owned-text output slot. */
        return FVM_INVALID_ARGUMENT;                               /* Require an owned-text output slot. */
    } /* End conditional branch: Require an owned-text output slot. */
    *out = 0;             /* Publish no partial parse allocation. */
    if (!json)            /* Reject absent provider output. */
    {                     /* Begin conditional branch: Reject absent provider output. */
        return FVM_ERROR; /* Reject absent provider output. */
    } /* End conditional branch: Reject absent provider output. */
    size_t length = 0;                                      /* Bound provider-controlled JSON storage. */
    while (length <= FVM_SPEECH_TEXT_LIMIT && json[length]) /* Require a terminator within the accepted transcript bound. */
    {                                                       /* Begin loop body: Require a terminator within the accepted transcript bound. */
        ++length;                                           /* Require a terminator within the accepted transcript bound. */
    } /* End loop body: Require a terminator within the accepted transcript bound. */
    if (length > FVM_SPEECH_TEXT_LIMIT) /* Reject unreasonable provider output before allocation. */
    {                                   /* Begin conditional branch: Reject unreasonable provider output before allocation. */
        return FVM_ERROR;               /* Reject unreasonable provider output before allocation. */
    } /* End conditional branch: Reject unreasonable provider output before allocation. */
    const char *cursor = json; /* Traverse actual provider JSON. */
    skip_space(cursor);        /* Permit legal leading JSON whitespace. */
    if (*cursor++ != '{')      /* Require the configured object shape. */
    {                          /* Begin conditional branch: Require the configured object shape. */
        return FVM_ERROR;      /* Require the configured object shape. */
    } /* End conditional branch: Require the configured object shape. */
    skip_space(cursor);                     /* Locate the sole result property. */
    const char *key =                       /* Select the exact native result key requested by the caller. */
        final ? "\"text\"" : "\"partial\""; /* Select the exact native result key requested by the caller. */
    size_t key_length = strlen(key);        /* Count the fixed expected JSON key. */
    if (strncmp(cursor, key, key_length))   /* Reject unrelated objects or enabled extra-result schemas. */
    {                                       /* Begin conditional branch: Reject unrelated objects or enabled extra-result schemas. */
        return FVM_ERROR;                   /* Reject unrelated objects or enabled extra-result schemas. */
    } /* End conditional branch: Reject unrelated objects or enabled extra-result schemas. */
    cursor += key_length; /* Advance beyond the validated result key. */
    skip_space(cursor);   /* Locate the property separator. */
    if (*cursor++ != ':') /* Require ordinary JSON key/value framing. */
    {                     /* Begin conditional branch: Require ordinary JSON key/value framing. */
        return FVM_ERROR; /* Require ordinary JSON key/value framing. */
    } /* End conditional branch: Require ordinary JSON key/value framing. */
    skip_space(cursor);   /* Locate the result string. */
    if (*cursor++ != '"') /* Require actual text rather than another JSON value type. */
    {                     /* Begin conditional branch: Require actual text rather than another JSON value type. */
        return FVM_ERROR; /* Require actual text rather than another JSON value type. */
    } /* End conditional branch: Require actual text rather than another JSON value type. */
    char *text =                             /* Decoded JSON string bytes cannot exceed the original source length. */
        new (std::nothrow) char[length + 1]; /* Decoded JSON string bytes cannot exceed the original source length. */
    if (!text)                               /* Preserve allocation failure before parsing. */
    {                                        /* Begin conditional branch: Preserve allocation failure before parsing. */
        return FVM_OUT_OF_MEMORY;            /* Preserve allocation failure before parsing. */
    } /* End conditional branch: Preserve allocation failure before parsing. */
    size_t used = 0;                                    /* Track initialized decoded bytes. */
    bool valid = true;                                  /* Preserve syntax/Unicode failure without leaking partial ownership. */
    while (*cursor && *cursor != '"' && valid)          /* Decode the complete bounded result string. */
    {                                                   /* Begin one plain byte or escape. */
        unsigned char value = (unsigned char)*cursor++; /* Consume one in-bounds JSON string byte. */
        if (value < FVM_SPEECH_JSON_CONTROL_LIMIT)      /* Reject unescaped control bytes. */
        {                                               /* Begin conditional branch: Reject unescaped control bytes. */
            valid = false;                              /* Reject unescaped control bytes. */
        } /* End conditional branch: Reject unescaped control bytes. */
        else if (value != '\\')         /* Preserve ordinary UTF-8 for final scalar validation. */
        {                               /* Begin conditional branch: Preserve ordinary UTF-8 for final scalar validation. */
            text[used++] = (char)value; /* Preserve ordinary UTF-8 for final scalar validation. */
        } /* End conditional branch: Preserve ordinary UTF-8 for final scalar validation. */
        else                       /* Decode JSON's defined escape syntax. */
        {                          /* Begin an escaped code unit or punctuation character. */
            char escape = *cursor; /* Inspect before consuming the escape byte. */
            if (escape)            /* Never advance beyond a truncated source terminator. */
            {                      /* Begin conditional branch: Never advance beyond a truncated source terminator. */
                ++cursor;          /* Never advance beyond a truncated source terminator. */
            } /* End conditional branch: Never advance beyond a truncated source terminator. */
            if (escape == '"' || escape == '\\' || escape == '/') /* Preserve escaped JSON punctuation as plain text. */
            {                                                     /* Begin conditional branch: Preserve escaped JSON punctuation as plain text. */
                text[used++] = escape;                            /* Preserve escaped JSON punctuation as plain text. */
            } /* End conditional branch: Preserve escaped JSON punctuation as plain text. */
            else if (escape == 'b')  /* Decode backspace exactly. */
            {                        /* Begin conditional branch: Decode backspace exactly. */
                text[used++] = '\b'; /* Decode backspace exactly. */
            } /* End conditional branch: Decode backspace exactly. */
            else if (escape == 'f')  /* Decode form feed exactly. */
            {                        /* Begin conditional branch: Decode form feed exactly. */
                text[used++] = '\f'; /* Decode form feed exactly. */
            } /* End conditional branch: Decode form feed exactly. */
            else if (escape == 'n')  /* Decode newline exactly. */
            {                        /* Begin conditional branch: Decode newline exactly. */
                text[used++] = '\n'; /* Decode newline exactly. */
            } /* End conditional branch: Decode newline exactly. */
            else if (escape == 'r')  /* Decode carriage return exactly. */
            {                        /* Begin conditional branch: Decode carriage return exactly. */
                text[used++] = '\r'; /* Decode carriage return exactly. */
            } /* End conditional branch: Decode carriage return exactly. */
            else if (escape == 't')  /* Decode horizontal tab exactly. */
            {                        /* Begin conditional branch: Decode horizontal tab exactly. */
                text[used++] = '\t'; /* Decode horizontal tab exactly. */
            } /* End conditional branch: Decode horizontal tab exactly. */
            else if (escape == 'u')                                     /* Decode a scalar or a correctly paired UTF-16 surrogate escape. */
            {                                                           /* Begin Unicode escape conversion. */
                unsigned long scalar = 0;                               /* Receive the first UTF-16 code unit. */
                valid = read_unit(cursor, scalar);                      /* Reject malformed four-digit escapes. */
                if (valid && scalar >= FVM_UTF16_HIGH_SURROGATE_BASE && /* Require a trailing surrogate after a leading one. */
                    scalar <= FVM_UTF16_HIGH_SURROGATE_MAX)             /* Require a trailing surrogate after a leading one. */
                {                                                       /* Decode one supplementary Unicode scalar. */
                    if (*cursor != '\\' || cursor[1] != 'u')            /* Short-circuit before inspecting beyond a source terminator. */
                    {                                                   /* Begin conditional branch: Short-circuit before inspecting beyond a source terminator. */
                        valid = false;                                  /* Short-circuit before inspecting beyond a source terminator. */
                    } /* End conditional branch: Short-circuit before inspecting beyond a source terminator. */
                    else                                                                                               /* Consume the complete trailing Unicode escape. */
                    {                                                                                                  /* Begin low-surrogate extraction. */
                        cursor += 2;                                                                                   /* Skip the already validated escape marker. */
                        unsigned long low = 0;                                                                         /* Receive the trailing UTF-16 code unit. */
                        valid = read_unit(cursor, low) && low >= FVM_UTF16_LOW_SURROGATE_BASE &&                       /* Require a genuine low surrogate. */
                                low <= FVM_UTF16_LOW_SURROGATE_MAX;                                                    /* Require a genuine low surrogate. */
                        if (valid)                                                                                     /* Combine the actual surrogate pair. */
                        {                                                                                              /* Begin conditional branch: Combine the actual surrogate pair. */
                            scalar = FVM_UTF16_SUPPLEMENTARY_OFFSET +                                                  /* Combine the actual surrogate pair. */
                                     ((scalar - FVM_UTF16_HIGH_SURROGATE_BASE) << FVM_UTF16_PAIR_PAYLOAD_BITS) + low - /* Combine the actual surrogate pair. */
                                     FVM_UTF16_LOW_SURROGATE_BASE;                                                     /* Combine the actual surrogate pair. */
                        } /* End conditional branch: Combine the actual surrogate pair. */
                    } /* Finish low-surrogate extraction. */
                } /* Finish supplementary scalar decoding. */
                size_t bytes = 0;                                                                        /* Receive the actual scalar UTF-8 width. */
                if (valid)                                                                               /* Reject NUL, lone low surrogates and invalid Unicode. */
                {                                                                                        /* Begin conditional branch: Reject NUL, lone low surrogates and invalid Unicode. */
                    valid = scalar != 0 && fvm_utf8_write(scalar, text + used, length - used, &bytes) == /* Reject NUL, lone low surrogates and invalid Unicode. */
                                               FVM_UTF8_OK;                                              /* Reject NUL, lone low surrogates and invalid Unicode. */
                } /* End conditional branch: Reject NUL, lone low surrogates and invalid Unicode. */
                used += bytes; /* Advance only across actually encoded scalar bytes. */
            } /* Finish Unicode escape conversion. */
            else               /* Reject unknown or truncated escapes. */
            {                  /* Begin conditional branch: Reject unknown or truncated escapes. */
                valid = false; /* Reject unknown or truncated escapes. */
            } /* End conditional branch: Reject unknown or truncated escapes. */
        } /* Finish escape decoding. */
    } /* Finish bounded string decoding. */
    if (*cursor != '"') /* Require a complete closing quote. */
    {                   /* Begin conditional branch: Require a complete closing quote. */
        valid = false;  /* Require a complete closing quote. */
    } /* End conditional branch: Require a complete closing quote. */
    else          /* Consume only a validated closing delimiter. */
    {             /* Begin conditional branch: Consume only a validated closing delimiter. */
        ++cursor; /* Consume only a validated closing delimiter. */
    } /* End conditional branch: Consume only a validated closing delimiter. */
    skip_space(cursor); /* Locate the configured object's final delimiter. */
    if (*cursor != '}') /* Reject extra properties and unexpected JSON shapes. */
    {                   /* Begin conditional branch: Reject extra properties and unexpected JSON shapes. */
        valid = false;  /* Reject extra properties and unexpected JSON shapes. */
    } /* End conditional branch: Reject extra properties and unexpected JSON shapes. */
    else          /* Consume the object terminator safely. */
    {             /* Begin conditional branch: Consume the object terminator safely. */
        ++cursor; /* Consume the object terminator safely. */
    } /* End conditional branch: Consume the object terminator safely. */
    skip_space(cursor);                                             /* Permit legal trailing JSON whitespace only. */
    if (*cursor || fvm_utf8_validate(text, used, 0) != FVM_UTF8_OK) /* Reject trailing tokens and invalid literal Unicode bytes. */
    {                                                               /* Begin conditional branch: Reject trailing tokens and invalid literal Unicode bytes. */
        valid = false;                                              /* Reject trailing tokens and invalid literal Unicode bytes. */
    } /* End conditional branch: Reject trailing tokens and invalid literal Unicode bytes. */
    if (!valid || !used)             /* Release invalid or empty recognition output. */
    {                                /* Do not publish partial or fabricated text. */
        delete[] text;               /* Release the decoder-owned temporary allocation. */
        return valid ? FVM_NOT_FOUND /* Distinguish actual empty hypotheses from malformed provider data. */
                     : FVM_ERROR;    /* Distinguish actual empty hypotheses from malformed provider data. */
    } /* Finish empty/error cleanup. */
    text[used] = 0; /* Complete host-compatible plain UTF-8 termination. */
    *out = text;    /* Transfer decoded transcript ownership to the native session. */
    return FVM_OK;  /* Return actual recognized text rather than its JSON envelope. */
} /* Finish transcript extraction. */
