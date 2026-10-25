%{
/* Declare standard I/O used by the scanner and grammar action. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define DEMO_NUMBER_MAX 1000000 /* Bound token values before decimal accumulation can overflow. */


/* Declare yyparse before main; the unchanged Inferno skeleton defines it later. */
int yyparse(void);


/* Adapt only yaccpar's two numeric-name formats to its documented 16-byte local buffers.
 * buffer points to that skeleton storage, format is borrowed, varargs contains one int.
 * Returns the bounded formatted count, or -1 with a diagnostic for another format. */
int demo_sprint(char *buffer, char *format, ...)
{
    va_list arguments; /* Access the one integer supplied by yaccpar. */
    int value; /* Token/state code to print. */
    if (strcmp(format, "<%d>") != 0 && strcmp(format, "<%d>\n") != 0)
    {
        fprintf(stderr, "unsupported tutorial skeleton format\n"); /* Reject a pretend lib9 implementation. */
        return -1; /* Preserve the unsupported-format failure. */
    }
    va_start(arguments, format); /* Start the validated single-value format. */
    value = va_arg(arguments, int); /* Both known yaccpar calls pass int. */
    va_end(arguments); /* Release variadic traversal state. */
    return snprintf(buffer, 16, format, value); /* Match the skeleton's actual buffer size. */
}

#define sprint demo_sprint /* Only this generated tutorial adapts the Inferno numeric-name helper. */


/* Read one number or punctuation token; no parameters, returns a yacc token or EOF. */
int yylex(void);


/* Report a parser error; message is borrowed, returns normally to yyparse's error path. */
void yyerror(const char *message);
%}
/* The generated header assigns a token code and supplies the default int semantic value. */
%token NUMBER
/* Begin the demonstration grammar, independent from all new Coco/R compiler parsers. */
%%
start: NUMBER
    {
        printf("%d\n", $1); /* Print the admitted numeric token. */
    }
;

/* End productions and begin the C host part of the teaching parser. */
%%


/* Read decimal NUMBER tokens with an explicit bound, or return punctuation unchanged.
 * No parameters. Returns zero at EOF, NUMBER with yylval, or a character token. */
int yylex(void)
{
    int c; /* Current input byte or EOF. */
    int value = 0; /* Accumulated bounded decimal value. */
    do
    {
        c = getchar(); /* Read from the tutorial process stdin. */
    } while (c == ' ' || c == '\n' || c == '\t' || c == '\r');
    if (c < '0' || c > '9') /* Punctuation remains available to the grammar. */
    {
        return c == EOF ? 0 : c; /* yacc uses zero as end of input. */
    }
    do
    {
        if (value > (DEMO_NUMBER_MAX - (c - '0')) / 10) /* Reject before overflowing int. */
        {
            yyerror("number exceeds tutorial bound"); /* Explain invalid input. */
            return '?'; /* Let yyparse reject the unrecognized token. */
        }
        value = value * 10 + c - '0'; /* Accumulate one admitted decimal digit. */
        c = getchar(); /* Inspect the next input byte. */
    } while (c >= '0' && c <= '9');
    if (c != EOF) /* Do not push EOF back into the input stream. */
    {
        ungetc(c, stdin); /* Preserve following punctuation for the next scanner call. */
    }
    yylval = value; /* Publish the token's semantic value to the generated parser. */
    return NUMBER; /* Report the declared token kind. */
}


/* Print the generated parser's error diagnostic without assertion-dependent behavior.
 * message is borrowed. Returns nothing; yyparse decides the failure status. */
void yyerror(const char *message)
{
    fprintf(stderr, "iyacc example: %s\n", message); /* Keep errors separate from parsed output. */
}


/* Run the generated parser over stdin. No parameters; returns its success/failure status. */
int main(void)
{
    return yyparse(); /* Preserve the real parser exit code for shell automation. */
}
