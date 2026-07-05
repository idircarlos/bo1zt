/***********************************************************************************
*
*   argparse.h - A single-header C library for command-line argument parsing,
*                inspired by Python's argparse module.
*
*   USAGE:
*       In ONE C file (the implementation file), define ARGPARSE_IMPLEMENTATION
*       BEFORE including this header:
*
*           #define ARGPARSE_IMPLEMENTATION
*           #include "argparse.h"
*
*       In every other file that wants to use the library, just include
*       "argparse.h" without defining the macro.
*
*   QUICK EXAMPLE:
*
*       Parser *p = NewParser("myprog");
*       SetDescription(p, "What the program does");
*
*       AddArgument(p, "filename", NULL);                  // positional
*       ArgAction(AddArgument(p, "-v", "--verbose", NULL), // flag
*                 ACTION_STORE_TRUE);
*       ArgType(AddArgument(p, "-c", "--count", NULL),     // typed option
*               TYPE_INT);
*
*       Namespace *ns = ParseArgs(p, argc, argv);
*
*       printf("file=%s verbose=%d count=%ld\n",
*              GetString(ns, "filename"),
*              GetBool(ns, "verbose"),
*              GetInt(ns, "count"));
*
*       FreeNamespace(ns);
*       FreeParser(p);
*
***********************************************************************************/

#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#ifndef ARGPARSEAPI
    #define ARGPARSEAPI extern
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */

#define ARGPARSE_VERSION_MAJOR 1
#define ARGPARSE_VERSION_MINOR 0
#define ARGPARSE_VERSION_PATCH 0

/* Sentinel that, when used as a "default" value, suppresses adding the
   attribute to the namespace if it was never seen. Mirrors argparse.SUPPRESS. */
#define SUPPRESS ((const char *)"==SUPPRESS==")

/* ---------------------------------------------------------------------------
 * Enums
 * --------------------------------------------------------------------------- */

typedef enum {
    TYPE_STRING = 0,  /* default - keep as string */
    TYPE_INT,         /* parse as long via strtol */
    TYPE_FLOAT,       /* parse as double via strtod */
    TYPE_BOOL         /* parse as bool (true/false/1/0/yes/no) */
} ValueKind;

typedef enum {
    ACTION_STORE = 0,      /* default: store the value */
    ACTION_STORE_CONST,    /* store a constant value */
    ACTION_STORE_TRUE,     /* store boolean true */
    ACTION_STORE_FALSE,    /* store boolean false */
    ACTION_APPEND,         /* append value to a list */
    ACTION_APPEND_CONST,   /* append constant to a list */
    ACTION_COUNT,          /* count occurrences */
    ACTION_HELP,           /* print help and exit */
    ACTION_VERSION         /* print version and exit */
} ActionKind;

typedef enum {
    NARGS_NONE = 0,        /* default: 1 value (or 0 for flag actions) */
    NARGS_N,               /* exact count given via ArgNargsN */
    NARGS_OPTIONAL,        /* '?' - 0 or 1 */
    NARGS_ZERO_OR_MORE,    /* '*' - 0 or more */
    NARGS_ONE_OR_MORE,     /* '+' - 1 or more */
    NARGS_REMAINDER        /* consume the rest of argv */
} NargsKind;

/* ---------------------------------------------------------------------------
 * Opaque types
 * --------------------------------------------------------------------------- */

typedef struct Parser     Parser;
typedef struct Argument   Argument;
typedef struct Namespace  Namespace;
typedef struct Subparsers Subparsers;

/* ---------------------------------------------------------------------------
 * Parser construction and configuration
 * --------------------------------------------------------------------------- */

ARGPARSEAPI Parser *NewParser(const char *prog);
ARGPARSEAPI void    FreeParser(Parser *p);

ARGPARSEAPI void SetProg         (Parser *p, const char *prog);
ARGPARSEAPI void SetDescription  (Parser *p, const char *description);
ARGPARSEAPI void SetEpilog       (Parser *p, const char *epilog);
ARGPARSEAPI void SetUsage        (Parser *p, const char *usage);
ARGPARSEAPI void SetPrefixChars  (Parser *p, const char *prefix_chars);
ARGPARSEAPI void SetAddHelp      (Parser *p, bool add_help);
ARGPARSEAPI void SetExitOnError  (Parser *p, bool exit_on_error);
ARGPARSEAPI void SetAllowAbbrev  (Parser *p, bool allow_abbrev);
ARGPARSEAPI void SetVersion      (Parser *p, const char *version);

/* ---------------------------------------------------------------------------
 * AddArgument: register a new argument
 *
 *   For positional args:  AddArgument(p, "filename", NULL);
 *   For optional args:    AddArgument(p, "-c", "--count", NULL);
 *
 * The variadic list MUST be terminated with a NULL.
 * --------------------------------------------------------------------------- */

ARGPARSEAPI Argument *AddArgument(Parser *p, const char *first, ...);

/* ---------------------------------------------------------------------------
 * Argument configuration (chainable: each returns the same arg pointer).
 * --------------------------------------------------------------------------- */

ARGPARSEAPI Argument *ArgHelp     (Argument *a, const char *help);
ARGPARSEAPI Argument *ArgDefault  (Argument *a, const char *def);
ARGPARSEAPI Argument *ArgType     (Argument *a, ValueKind type);
ARGPARSEAPI Argument *ArgAction   (Argument *a, ActionKind action);
ARGPARSEAPI Argument *ArgNargs    (Argument *a, NargsKind nargs);
ARGPARSEAPI Argument *ArgNargsN   (Argument *a, int n);
ARGPARSEAPI Argument *ArgRequired (Argument *a, bool required);
ARGPARSEAPI Argument *ArgChoices  (Argument *a, const char *first, ...);
ARGPARSEAPI Argument *ArgMetavar  (Argument *a, const char *metavar);
ARGPARSEAPI Argument *ArgDest     (Argument *a, const char *dest);
ARGPARSEAPI Argument *ArgConst    (Argument *a, const char *value);

/* ---------------------------------------------------------------------------
 * Subparsers (subcommand support)
 * --------------------------------------------------------------------------- */

ARGPARSEAPI Subparsers *AddSubparsers     (Parser *p, const char *dest);
ARGPARSEAPI void        SetSubparsersRequired(Subparsers *sp, bool required);
ARGPARSEAPI void        SetSubparsersHelp    (Subparsers *sp, const char *help);
ARGPARSEAPI void        SetSubparsersTitle   (Subparsers *sp, const char *title);
ARGPARSEAPI Parser     *AddParser            (Subparsers *sp, const char *name, const char *help);

/* ---------------------------------------------------------------------------
 * Parsing
 * --------------------------------------------------------------------------- */

/* Parse all arguments. Returns NULL on error (when exit_on_error=false).
   When exit_on_error=true (default), errors call exit(2). */
ARGPARSEAPI Namespace *ParseArgs(Parser *p, int argc, char **argv);

/* Like ParseArgs, but allows leftover unknown arguments. The caller owns
   the returned 'unknown' array (NULL-terminated) and must free it (but not
   the strings within, which point into argv). */
ARGPARSEAPI Namespace *ParseKnownArgs(Parser *p, int argc, char **argv,
                                      int *out_unknown_count, char ***out_unknown);

/* ---------------------------------------------------------------------------
 * Namespace access
 * --------------------------------------------------------------------------- */

ARGPARSEAPI bool        Has         (const Namespace *ns, const char *name);
ARGPARSEAPI bool        WasPresent  (const Namespace *ns, const char *name);
ARGPARSEAPI const char *GetString   (const Namespace *ns, const char *name);
ARGPARSEAPI long        GetInt      (const Namespace *ns, const char *name);
ARGPARSEAPI double      GetFloat    (const Namespace *ns, const char *name);
ARGPARSEAPI bool        GetBool     (const Namespace *ns, const char *name);
ARGPARSEAPI size_t      GetCount    (const Namespace *ns, const char *name);
ARGPARSEAPI const char *GetStringAt (const Namespace *ns, const char *name, size_t idx);
ARGPARSEAPI long        GetIntAt    (const Namespace *ns, const char *name, size_t idx);
ARGPARSEAPI double      GetFloatAt  (const Namespace *ns, const char *name, size_t idx);

/* For subcommands. Returns the chosen subcommand name (or NULL) and its
   sub-namespace. Both are owned by 'ns' and freed when 'ns' is freed. */
ARGPARSEAPI const char      *GetSubcommand   (const Namespace *ns);
ARGPARSEAPI const Namespace *GetSubnamespace (const Namespace *ns);

ARGPARSEAPI void FreeNamespace(Namespace *ns);

/* ---------------------------------------------------------------------------
 * Help, usage, errors
 * --------------------------------------------------------------------------- */

ARGPARSEAPI void  PrintHelp  (const Parser *p, FILE *out);
ARGPARSEAPI void  PrintUsage (const Parser *p, FILE *out);
ARGPARSEAPI char *FormatHelp (const Parser *p);
ARGPARSEAPI char *FormatUsage(const Parser *p);

/* Report an error in the standard "prog: error: ..." form. If exit_on_error
   is set on the parser, it prints usage to stderr and exit(2). Otherwise it
   stores the message internally (retrievable via GetError). */
ARGPARSEAPI void        Error    (Parser *p, const char *fmt, ...);
ARGPARSEAPI const char *GetError (const Parser *p);

#ifdef __cplusplus
}
#endif

#endif /* ARGPARSE_H_ */

/***********************************************************************************
*
*   ARGPARSE IMPLEMENTATION
*
*   To get the implementation, define ARGPARSE_IMPLEMENTATION in EXACTLY ONE
*   source file before including this header.
*
************************************************************************************/

#ifdef ARGPARSE_IMPLEMENTATION
#ifndef ARGPARSE_IMPLEMENTATION_INCLUDED_
#define ARGPARSE_IMPLEMENTATION_INCLUDED_

#include <limits.h>


/* ===========================================================================
 *  Internal data structures
 * =========================================================================== */

#define AP_DEFAULT_PREFIX_CHARS "-"

/* Dynamic string array */
typedef struct {
    char  **items;
    size_t  count;
    size_t  capacity;
} ApStrvec;

struct Argument {
    /* Option strings for optional args (e.g. "-v", "--verbose").
       For positionals, this contains a single entry: the positional name. */
    ApStrvec  option_strings;

    bool         is_positional;
    char        *dest;
    char        *help;
    char        *metavar;
    char        *default_value;
    char        *const_value;

    ValueKind    type;
    ActionKind   action;
    NargsKind    nargs;
    int          nargs_n;

    bool         required;
    bool         required_explicit;

    ApStrvec   choices;

    bool         seen;
};

typedef struct ApArglist {
    Argument **items;
    size_t     count;
    size_t     capacity;
} ApArglist;

struct Subparsers {
    char       *dest;
    char       *title;
    char       *help;
    bool        required;

    ApStrvec  names;
    ApStrvec  helps;
    Parser    **parsers;
    size_t      capacity;

    Parser     *parent;
};

struct Parser {
    char        *prog;
    char        *description;
    char        *epilog;
    char        *usage;
    char        *prefix_chars;
    char        *version;
    bool         add_help;
    bool         exit_on_error;
    bool         allow_abbrev;
    bool         help_added;

    ApArglist  args;
    Subparsers  *subparsers;

    char        *error_msg;
};

typedef struct ApValue {
    char       *name;
    bool        is_list;
    bool        present;
    char       *scalar;
    ApStrvec  list;
    long        counter;
    bool        is_counter;
} ApValue;

typedef struct ApValvec {
    ApValue **items;
    size_t      count;
    size_t      capacity;
} ApValvec;

struct Namespace {
    ApValvec  values;
    char       *subcommand;
    Namespace  *sub_namespace;
};

/* ===========================================================================
 *  Internal helpers - memory + strings
 * =========================================================================== */

static void *ApXmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "argparse: out of memory\n"); abort(); }
    return p;
}

static void *ApXcalloc(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) { fprintf(stderr, "argparse: out of memory\n"); abort(); }
    return p;
}

static void *ApXrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) { fprintf(stderr, "argparse: out of memory\n"); abort(); }
    return q;
}

static char *ApStrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *r = (char *)ApXmalloc(n);
    memcpy(r, s, n);
    return r;
}

static char *ApStrdupN(const char *s, size_t n) {
    char *r = (char *)ApXmalloc(n + 1);
    memcpy(r, s, n);
    r[n] = 0;
    return r;
}

static void ApFree(void *p) { free(p); }

/* Vector of strings */
static void ApStrvecInit(ApStrvec *v) {
    v->items = NULL; v->count = 0; v->capacity = 0;
}

static void ApStrvecPush(ApStrvec *v, const char *s) {
    if (v->count + 1 > v->capacity) {
        v->capacity = v->capacity ? v->capacity * 2 : 4;
        v->items = (char **)ApXrealloc(v->items, v->capacity * sizeof(char *));
    }
    v->items[v->count++] = ApStrdup(s);
}

static void ApStrvecClear(ApStrvec *v) {
    for (size_t i = 0; i < v->count; ++i) ApFree(v->items[i]);
    ApFree(v->items);
    ApStrvecInit(v);
}

static bool ApStrvecContains(const ApStrvec *v, const char *s) {
    for (size_t i = 0; i < v->count; ++i)
        if (strcmp(v->items[i], s) == 0) return true;
    return false;
}

/* ===========================================================================
 *  Internal helpers - identifying option strings
 * =========================================================================== */

static bool ApIsPrefixChar(const Parser *p, char c) {
    const char *pc = p->prefix_chars ? p->prefix_chars : AP_DEFAULT_PREFIX_CHARS;
    return strchr(pc, c) != NULL;
}

static bool ApIsOptionString(const Parser *p, const char *s) {
    if (!s || !s[0]) return false;
    if (!ApIsPrefixChar(p, s[0])) return false;
    if (s[1] == '\0') return false;
    return true;
}

static bool ApIsLongOption(const Parser *p, const char *s) {
    if (!s || strlen(s) < 2) return false;
    return ApIsPrefixChar(p, s[0]) && s[0] == s[1];
}

static bool ApLooksNumeric(const char *s) {
    if (!s || !*s) return false;
    const char *q = s;
    if (*q == '-' || *q == '+') q++;
    if (!*q) return false;
    bool seen_digit = false, seen_dot = false, seen_e = false;
    for (; *q; ++q) {
        if (isdigit((unsigned char)*q)) { seen_digit = true; }
        else if (*q == '.' && !seen_dot && !seen_e) { seen_dot = true; }
        else if ((*q == 'e' || *q == 'E') && seen_digit && !seen_e) { seen_e = true; seen_digit = false; }
        else if ((*q == '+' || *q == '-') && seen_e && q != s) {
            if (*(q-1) != 'e' && *(q-1) != 'E') return false;
        }
        else return false;
    }
    return seen_digit;
}

static char *ApMakeDest(const Parser *p, const ApStrvec *opts, bool is_positional) {
    if (is_positional) return ApStrdup(opts->items[0]);

    const char *chosen = NULL;
    for (size_t i = 0; i < opts->count; ++i) {
        if (ApIsLongOption(p, opts->items[i])) { chosen = opts->items[i]; break; }
    }
    if (!chosen) chosen = opts->items[0];

    const char *q = chosen;
    while (*q && ApIsPrefixChar(p, *q)) q++;

    char *dest = ApStrdup(q);
    for (char *c = dest; *c; ++c) if (*c == '-') *c = '_';
    return dest;
}

/* ===========================================================================
 *  Internal helpers - argument list
 * =========================================================================== */

static void ApArglistInit(ApArglist *al) {
    al->items = NULL; al->count = 0; al->capacity = 0;
}

static void ApArglistPush(ApArglist *al, Argument *a) {
    if (al->count + 1 > al->capacity) {
        al->capacity = al->capacity ? al->capacity * 2 : 4;
        al->items = (Argument **)ApXrealloc(al->items, al->capacity * sizeof(*al->items));
    }
    al->items[al->count++] = a;
}

static void ApArgumentFree(Argument *a) {
    if (!a) return;
    ApStrvecClear(&a->option_strings);
    ApStrvecClear(&a->choices);
    ApFree(a->dest);
    ApFree(a->help);
    ApFree(a->metavar);
    ApFree(a->default_value);
    ApFree(a->const_value);
    ApFree(a);
}

/* ===========================================================================
 *  Internal helpers - namespace values
 * =========================================================================== */

static ApValue *ApValueNew(const char *name) {
    ApValue *v = (ApValue *)ApXcalloc(1, sizeof(*v));
    v->name = ApStrdup(name);
    ApStrvecInit(&v->list);
    return v;
}

static void ApValueFree(ApValue *v) {
    if (!v) return;
    ApFree(v->name);
    ApFree(v->scalar);
    ApStrvecClear(&v->list);
    ApFree(v);
}

static void ApValvecInit(ApValvec *v) {
    v->items = NULL; v->count = 0; v->capacity = 0;
}

static void ApValvecPush(ApValvec *v, ApValue *val) {
    if (v->count + 1 > v->capacity) {
        v->capacity = v->capacity ? v->capacity * 2 : 4;
        v->items = (ApValue **)ApXrealloc(v->items, v->capacity * sizeof(*v->items));
    }
    v->items[v->count++] = val;
}

static ApValue *ApNsFind(const Namespace *ns, const char *name) {
    for (size_t i = 0; i < ns->values.count; ++i) {
        if (strcmp(ns->values.items[i]->name, name) == 0) return ns->values.items[i];
    }
    return NULL;
}

static ApValue *ApNsEnsure(Namespace *ns, const char *name) {
    ApValue *v = ApNsFind(ns, name);
    if (v) return v;
    v = ApValueNew(name);
    ApValvecPush(&ns->values, v);
    return v;
}

static void ApValueSetScalar(ApValue *v, const char *s) {
    ApFree(v->scalar);
    v->scalar = s ? ApStrdup(s) : NULL;
    v->is_list = false;
}

static void ApValueAppend(ApValue *v, const char *s) {
    v->is_list = true;
    ApStrvecPush(&v->list, s ? s : "");
}

/* ===========================================================================
 *  Internal helpers - validation
 * =========================================================================== */

static bool ApValidateChoice(const Argument *a, const char *value, char *errbuf, size_t errsz) {
    if (a->choices.count == 0) return true;
    if (ApStrvecContains(&a->choices, value)) return true;
    size_t pos = 0;
    pos += (size_t)snprintf(errbuf + pos, (pos < errsz ? errsz - pos : 0),
                            "argument %s: invalid choice: '%s' (choose from ",
                            a->dest ? a->dest : "?", value);
    for (size_t i = 0; i < a->choices.count; ++i) {
        if (pos < errsz) pos += (size_t)snprintf(errbuf + pos, errsz - pos, "%s'%s'",
                                                 i == 0 ? "" : ", ", a->choices.items[i]);
    }
    if (pos < errsz) snprintf(errbuf + pos, errsz - pos, ")");
    return false;
}

static bool ApValidateType(const Argument *a, const char *value, char *errbuf, size_t errsz) {
    char *endp = NULL;
    switch (a->type) {
        case TYPE_STRING: return true;
        case TYPE_INT: {
            (void)strtol(value, &endp, 10);
            if (endp == value || (endp && *endp != '\0')) {
                snprintf(errbuf, errsz, "argument %s: invalid int value: '%s'",
                         a->dest ? a->dest : "?", value);
                return false;
            }
            return true;
        }
        case TYPE_FLOAT: {
            (void)strtod(value, &endp);
            if (endp == value || (endp && *endp != '\0')) {
                snprintf(errbuf, errsz, "argument %s: invalid float value: '%s'",
                         a->dest ? a->dest : "?", value);
                return false;
            }
            return true;
        }
        case TYPE_BOOL: {
            const char *opts[] = { "true", "false", "1", "0", "yes", "no", "on", "off" };
            for (size_t i = 0; i < sizeof(opts)/sizeof(opts[0]); ++i)
                if (strcmp(value, opts[i]) == 0) return true;
            snprintf(errbuf, errsz, "argument %s: invalid bool value: '%s'",
                     a->dest ? a->dest : "?", value);
            return false;
        }
    }
    return true;
}

static bool ApStrToBool(const char *s) {
    if (!s) return false;
    if (strcmp(s, "true") == 0 || strcmp(s, "1") == 0 ||
        strcmp(s, "yes")  == 0 || strcmp(s, "on") == 0) return true;
    return false;
}

/* ===========================================================================
 *  Argument lookup
 * =========================================================================== */

static Argument *ApFindOptionalExact(const Parser *p, const char *opt) {
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (a->is_positional) continue;
        for (size_t j = 0; j < a->option_strings.count; ++j)
            if (strcmp(a->option_strings.items[j], opt) == 0) return a;
    }
    return NULL;
}

static Argument *ApFindOptionalAbbrev(const Parser *p, const char *opt,
                                          bool *ambiguous, char *amb_buf, size_t amb_sz) {
    *ambiguous = false;
    if (!ApIsLongOption(p, opt)) return NULL;
    size_t L = strlen(opt);
    Argument *match = NULL;
    char first_match[128] = {0};
    size_t pos = 0;
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (a->is_positional) continue;
        for (size_t j = 0; j < a->option_strings.count; ++j) {
            const char *cand = a->option_strings.items[j];
            if (!ApIsLongOption(p, cand)) continue;
            if (strncmp(cand, opt, L) == 0) {
                if (!match) { match = a; snprintf(first_match, sizeof(first_match), "%s", cand); }
                else if (match != a) {
                    *ambiguous = true;
                    if (amb_buf && amb_sz) {
                        if (pos == 0) pos = (size_t)snprintf(amb_buf, amb_sz, "%s", first_match);
                        if (pos < amb_sz) pos += (size_t)snprintf(amb_buf + pos, amb_sz - pos, ", %s", cand);
                    }
                }
            }
        }
    }
    if (*ambiguous) return NULL;
    return match;
}


/* ===========================================================================
 *  Parser construction
 * =========================================================================== */

ARGPARSEAPI Parser *NewParser(const char *prog) {
    Parser *p = (Parser *)ApXcalloc(1, sizeof(*p));
    p->prog          = ApStrdup(prog ? prog : "PROG");
    p->prefix_chars  = ApStrdup(AP_DEFAULT_PREFIX_CHARS);
    p->add_help      = true;
    p->exit_on_error = true;
    p->allow_abbrev  = true;
    p->help_added    = false;
    ApArglistInit(&p->args);
    return p;
}

static void ApSubparsersFree(Subparsers *sp);

ARGPARSEAPI void FreeParser(Parser *p) {
    if (!p) return;
    for (size_t i = 0; i < p->args.count; ++i) ApArgumentFree(p->args.items[i]);
    ApFree(p->args.items);
    ApSubparsersFree(p->subparsers);
    ApFree(p->prog);
    ApFree(p->description);
    ApFree(p->epilog);
    ApFree(p->usage);
    ApFree(p->prefix_chars);
    ApFree(p->version);
    ApFree(p->error_msg);
    ApFree(p);
}

ARGPARSEAPI void SetProg         (Parser *p, const char *s) { ApFree(p->prog);          p->prog         = ApStrdup(s); }
ARGPARSEAPI void SetDescription  (Parser *p, const char *s) { ApFree(p->description);   p->description  = ApStrdup(s); }
ARGPARSEAPI void SetEpilog       (Parser *p, const char *s) { ApFree(p->epilog);        p->epilog       = ApStrdup(s); }
ARGPARSEAPI void SetUsage        (Parser *p, const char *s) { ApFree(p->usage);         p->usage        = ApStrdup(s); }
ARGPARSEAPI void SetPrefixChars  (Parser *p, const char *s) { ApFree(p->prefix_chars);  p->prefix_chars = ApStrdup(s ? s : AP_DEFAULT_PREFIX_CHARS); }
ARGPARSEAPI void SetAddHelp      (Parser *p, bool b)        { p->add_help      = b; }
ARGPARSEAPI void SetExitOnError  (Parser *p, bool b)        { p->exit_on_error = b; }
ARGPARSEAPI void SetAllowAbbrev  (Parser *p, bool b)        { p->allow_abbrev  = b; }
ARGPARSEAPI void SetVersion      (Parser *p, const char *s) { ApFree(p->version);       p->version      = ApStrdup(s); }

/* ===========================================================================
 *  AddArgument
 * =========================================================================== */

ARGPARSEAPI Argument *AddArgument(Parser *p, const char *first, ...) {
    if (!p || !first) return NULL;

    Argument *a = (Argument *)ApXcalloc(1, sizeof(*a));
    ApStrvecInit(&a->option_strings);
    ApStrvecInit(&a->choices);
    a->type    = TYPE_STRING;
    a->action  = ACTION_STORE;
    a->nargs   = NARGS_NONE;
    a->nargs_n = 1;

    ApStrvecPush(&a->option_strings, first);
    va_list ap;
    va_start(ap, first);
    const char *next;
    while ((next = va_arg(ap, const char *)) != NULL) {
        ApStrvecPush(&a->option_strings, next);
    }
    va_end(ap);

    a->is_positional     = !ApIsOptionString(p, a->option_strings.items[0]);
    a->required          = a->is_positional;
    a->required_explicit = false;

    a->dest = ApMakeDest(p, &a->option_strings, a->is_positional);

    ApArglistPush(&p->args, a);
    return a;
}

/* ===========================================================================
 *  Argument configuration (chainable)
 * =========================================================================== */

ARGPARSEAPI Argument *ArgHelp(Argument *a, const char *help) {
    if (!a) return a;
    ApFree(a->help); a->help = ApStrdup(help);
    return a;
}

ARGPARSEAPI Argument *ArgDefault(Argument *a, const char *def) {
    if (!a) return a;
    ApFree(a->default_value); a->default_value = ApStrdup(def);
    return a;
}

ARGPARSEAPI Argument *ArgType(Argument *a, ValueKind type) {
    if (!a) return a;
    a->type = type;
    return a;
}

ARGPARSEAPI Argument *ArgAction(Argument *a, ActionKind action) {
    if (!a) return a;
    a->action = action;
    switch (action) {
        case ACTION_STORE_TRUE:
        case ACTION_STORE_FALSE:
        case ACTION_HELP:
        case ACTION_VERSION:
        case ACTION_COUNT:
        case ACTION_STORE_CONST:
        case ACTION_APPEND_CONST:
            a->nargs   = NARGS_NONE;
            a->nargs_n = 0;
            break;
        case ACTION_STORE:
        case ACTION_APPEND:
            break;
    }
    return a;
}

ARGPARSEAPI Argument *ArgNargs(Argument *a, NargsKind nargs) {
    if (!a) return a;
    a->nargs = nargs;
    if (a->is_positional && !a->required_explicit) {
        if (nargs == NARGS_OPTIONAL || nargs == NARGS_ZERO_OR_MORE ||
            nargs == NARGS_REMAINDER) {
            a->required = false;
        } else {
            a->required = true;
        }
    }
    return a;
}

ARGPARSEAPI Argument *ArgNargsN(Argument *a, int n) {
    if (!a) return a;
    a->nargs   = NARGS_N;
    a->nargs_n = n;
    return a;
}

ARGPARSEAPI Argument *ArgRequired(Argument *a, bool required) {
    if (!a) return a;
    a->required          = required;
    a->required_explicit = true;
    return a;
}

ARGPARSEAPI Argument *ArgChoices(Argument *a, const char *first, ...) {
    if (!a || !first) return a;
    ApStrvecClear(&a->choices);
    ApStrvecPush(&a->choices, first);
    va_list ap;
    va_start(ap, first);
    const char *next;
    while ((next = va_arg(ap, const char *)) != NULL) {
        ApStrvecPush(&a->choices, next);
    }
    va_end(ap);
    return a;
}

ARGPARSEAPI Argument *ArgMetavar(Argument *a, const char *metavar) {
    if (!a) return a;
    ApFree(a->metavar); a->metavar = ApStrdup(metavar);
    return a;
}

ARGPARSEAPI Argument *ArgDest(Argument *a, const char *dest) {
    if (!a) return a;
    ApFree(a->dest); a->dest = ApStrdup(dest);
    return a;
}

ARGPARSEAPI Argument *ArgConst(Argument *a, const char *value) {
    if (!a) return a;
    ApFree(a->const_value); a->const_value = ApStrdup(value);
    return a;
}


/* ===========================================================================
 *  Subparsers
 * =========================================================================== */

ARGPARSEAPI Subparsers *AddSubparsers(Parser *p, const char *dest) {
    if (!p) return NULL;
    if (p->subparsers) return p->subparsers;
    Subparsers *sp = (Subparsers *)ApXcalloc(1, sizeof(*sp));
    sp->dest     = ApStrdup(dest ? dest : "command");
    sp->required = false;
    sp->parent   = p;
    ApStrvecInit(&sp->names);
    ApStrvecInit(&sp->helps);
    sp->parsers  = NULL;
    sp->capacity = 0;
    p->subparsers = sp;
    return sp;
}

ARGPARSEAPI void SetSubparsersRequired(Subparsers *sp, bool required) {
    if (sp) sp->required = required;
}

ARGPARSEAPI void SetSubparsersHelp(Subparsers *sp, const char *help) {
    if (!sp) return;
    ApFree(sp->help); sp->help = ApStrdup(help);
}

ARGPARSEAPI void SetSubparsersTitle(Subparsers *sp, const char *title) {
    if (!sp) return;
    ApFree(sp->title); sp->title = ApStrdup(title);
}

ARGPARSEAPI Parser *AddParser(Subparsers *sp, const char *name, const char *help) {
    if (!sp || !name) return NULL;
    Parser *child = NewParser(name);
    if (sp->parent && sp->parent->prog) {
        size_t need = strlen(sp->parent->prog) + 1 + strlen(name) + 1;
        char *prog = (char *)ApXmalloc(need);
        snprintf(prog, need, "%s %s", sp->parent->prog, name);
        ApFree(child->prog);
        child->prog = prog;
    }
    child->exit_on_error = sp->parent ? sp->parent->exit_on_error : true;
    child->allow_abbrev  = sp->parent ? sp->parent->allow_abbrev  : true;

    if (sp->names.count + 1 > sp->capacity) {
        sp->capacity = sp->capacity ? sp->capacity * 2 : 4;
        sp->parsers = (Parser **)ApXrealloc(sp->parsers, sp->capacity * sizeof(*sp->parsers));
    }
    sp->parsers[sp->names.count] = child;
    ApStrvecPush(&sp->names, name);
    ApStrvecPush(&sp->helps, help ? help : "");
    return child;
}

static void ApSubparsersFree(Subparsers *sp) {
    if (!sp) return;
    for (size_t i = 0; i < sp->names.count; ++i) FreeParser(sp->parsers[i]);
    ApFree(sp->parsers);
    ApStrvecClear(&sp->names);
    ApStrvecClear(&sp->helps);
    ApFree(sp->dest);
    ApFree(sp->title);
    ApFree(sp->help);
    ApFree(sp);
}

/* ===========================================================================
 *  Help / usage formatting
 * =========================================================================== */

typedef struct { char *data; size_t len; size_t cap; } ApBuf;

static void ApBufInit(ApBuf *b)               { b->data = NULL; b->len = 0; b->cap = 0; }
static void ApBufReserve(ApBuf *b, size_t n)  {
    if (b->len + n + 1 > b->cap) {
        size_t newcap = b->cap ? b->cap : 64;
        while (newcap < b->len + n + 1) newcap *= 2;
        b->data = (char *)ApXrealloc(b->data, newcap);
        b->cap = newcap;
    }
}
static void ApBufAppend(ApBuf *b, const char *s) {
    size_t n = strlen(s);
    ApBufReserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = 0;
}
static void ApBufAppendf(ApBuf *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2; va_copy(ap2, ap);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0) { va_end(ap2); return; }
    ApBufReserve(b, (size_t)needed);
    vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap2);
    va_end(ap2);
    b->len += (size_t)needed;
}

static void ApBuildMetavar(const Argument *a, char *out, size_t n) {
    if (a->metavar && a->metavar[0]) { snprintf(out, n, "%s", a->metavar); return; }
    if (a->is_positional) { snprintf(out, n, "%s", a->dest); return; }
    size_t i = 0;
    for (; a->dest[i] && i + 1 < n; ++i) out[i] = (char)toupper((unsigned char)a->dest[i]);
    out[i] = 0;
}

static void ApRenderArgMetavar(const Argument *a, ApBuf *out) {
    char mv[128]; ApBuildMetavar(a, mv, sizeof(mv));
    switch (a->nargs) {
        case NARGS_NONE:
            if (a->action == ACTION_STORE_TRUE  || a->action == ACTION_STORE_FALSE ||
                a->action == ACTION_STORE_CONST || a->action == ACTION_APPEND_CONST ||
                a->action == ACTION_HELP        || a->action == ACTION_VERSION ||
                a->action == ACTION_COUNT) {
                /* no metavar */
            } else {
                ApBufAppendf(out, "%s", mv);
            }
            break;
        case NARGS_N: {
            for (int i = 0; i < a->nargs_n; ++i) {
                if (i) ApBufAppend(out, " ");
                ApBufAppend(out, mv);
            }
            break;
        }
        case NARGS_OPTIONAL:     ApBufAppendf(out, "[%s]", mv); break;
        case NARGS_ZERO_OR_MORE: ApBufAppendf(out, "[%s ...]", mv); break;
        case NARGS_ONE_OR_MORE:  ApBufAppendf(out, "%s [%s ...]", mv, mv); break;
        case NARGS_REMAINDER:    ApBufAppendf(out, "..."); break;
    }
}

static void ApRenderArgUsage(const Argument *a, ApBuf *out) {
    bool optional_wrap = !a->is_positional && !a->required;
    if (optional_wrap) ApBufAppend(out, "[");
    if (!a->is_positional) {
        const char *opt = a->option_strings.items[0];
        ApBufAppend(out, opt);
        ApBuf nm; ApBufInit(&nm);
        ApRenderArgMetavar(a, &nm);
        if (nm.len > 0) {
            ApBufAppend(out, " ");
            ApBufAppend(out, nm.data);
        }
        ApFree(nm.data);
    } else {
        ApBuf nm; ApBufInit(&nm);
        ApRenderArgMetavar(a, &nm);
        if (nm.len > 0) ApBufAppend(out, nm.data);
        else ApBufAppend(out, a->dest);
        ApFree(nm.data);
    }
    if (optional_wrap) ApBufAppend(out, "]");
}

static void ApRenderArgLabel(const Argument *a, ApBuf *out) {
    if (a->is_positional) {
        ApBuf nm; ApBufInit(&nm);
        ApRenderArgMetavar(a, &nm);
        if (nm.len > 0) ApBufAppend(out, nm.data);
        else ApBufAppend(out, a->dest);
        ApFree(nm.data);
        return;
    }
    ApBuf nm; ApBufInit(&nm);
    ApRenderArgMetavar(a, &nm);
    for (size_t i = 0; i < a->option_strings.count; ++i) {
        if (i) ApBufAppend(out, ", ");
        ApBufAppend(out, a->option_strings.items[i]);
        if (nm.len > 0) {
            ApBufAppend(out, " ");
            ApBufAppend(out, nm.data);
        }
    }
    ApFree(nm.data);
}


static void ApMaybeInjectHelp(Parser *p) {
    if (!p->add_help || p->help_added) return;
    char pc = p->prefix_chars && p->prefix_chars[0] ? p->prefix_chars[0] : '-';
    char shortopt[3] = { pc, 'h', 0 };
    char longopt[8];
    longopt[0] = pc; longopt[1] = pc;
    longopt[2] = 'h'; longopt[3] = 'e'; longopt[4] = 'l'; longopt[5] = 'p'; longopt[6] = 0;

    Argument *a = AddArgument(p, shortopt, longopt, NULL);
    ArgHelp(a, "show this help message and exit");
    ArgAction(a, ACTION_HELP);
    a->required          = false;
    a->required_explicit = true;
    if (p->args.count > 1) {
        Argument *last = p->args.items[p->args.count - 1];
        for (size_t i = p->args.count - 1; i > 0; --i)
            p->args.items[i] = p->args.items[i - 1];
        p->args.items[0] = last;
    }
    p->help_added = true;
}

ARGPARSEAPI char *FormatUsage(const Parser *p) {
    ApMaybeInjectHelp((Parser *)p);
    ApBuf out; ApBufInit(&out);
    ApBufAppend(&out, "usage: ");
    ApBufAppend(&out, p->prog ? p->prog : "PROG");
    if (p->usage) {
        ApBufAppend(&out, " ");
        ApBufAppend(&out, p->usage);
        ApBufAppend(&out, "\n");
        return out.data;
    }
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (a->is_positional) continue;
        ApBufAppend(&out, " ");
        ApRenderArgUsage(a, &out);
    }
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (!a->is_positional) continue;
        ApBufAppend(&out, " ");
        ApRenderArgUsage(a, &out);
    }
    if (p->subparsers) {
        ApBufAppend(&out, " {");
        for (size_t i = 0; i < p->subparsers->names.count; ++i) {
            if (i) ApBufAppend(&out, ",");
            ApBufAppend(&out, p->subparsers->names.items[i]);
        }
        ApBufAppend(&out, "} ...");
    }
    ApBufAppend(&out, "\n");
    return out.data;
}

static void ApHelpRow(ApBuf *out, const char *label, const char *help) {
    const size_t label_col = 24;
    ApBufAppend(out, "  ");
    ApBufAppend(out, label);
    size_t lablen = 2 + strlen(label);
    if (help && *help) {
        if (lablen + 2 > label_col) {
            ApBufAppend(out, "\n");
            for (size_t i = 0; i < label_col; ++i) ApBufAppend(out, " ");
        } else {
            for (size_t i = lablen; i < label_col; ++i) ApBufAppend(out, " ");
        }
        ApBufAppend(out, help);
    }
    ApBufAppend(out, "\n");
}

ARGPARSEAPI char *FormatHelp(const Parser *p) {
    ApMaybeInjectHelp((Parser *)p);
    ApBuf out; ApBufInit(&out);
    char *usage = FormatUsage(p);
    ApBufAppend(&out, usage);
    ApFree(usage);

    if (p->description && p->description[0]) {
        ApBufAppend(&out, "\n");
        ApBufAppend(&out, p->description);
        ApBufAppend(&out, "\n");
    }

    bool any_pos = false;
    for (size_t i = 0; i < p->args.count; ++i) if (p->args.items[i]->is_positional) { any_pos = true; break; }
    if (any_pos) {
        ApBufAppend(&out, "\npositional arguments:\n");
        for (size_t i = 0; i < p->args.count; ++i) {
            Argument *a = p->args.items[i];
            if (!a->is_positional) continue;
            ApBuf label; ApBufInit(&label);
            ApRenderArgLabel(a, &label);
            ApHelpRow(&out, label.data ? label.data : a->dest, a->help);
            ApFree(label.data);
        }
        if (p->subparsers) {
            ApBuf label; ApBufInit(&label);
            ApBufAppend(&label, "{");
            for (size_t i = 0; i < p->subparsers->names.count; ++i) {
                if (i) ApBufAppend(&label, ",");
                ApBufAppend(&label, p->subparsers->names.items[i]);
            }
            ApBufAppend(&label, "}");
            ApHelpRow(&out, label.data, p->subparsers->help);
            ApFree(label.data);
            for (size_t i = 0; i < p->subparsers->names.count; ++i) {
                ApBuf l2; ApBufInit(&l2);
                ApBufAppendf(&l2, "  %s", p->subparsers->names.items[i]);
                ApHelpRow(&out, l2.data, p->subparsers->helps.items[i]);
                ApFree(l2.data);
            }
        }
    } else if (p->subparsers) {
        ApBufAppend(&out, "\npositional arguments:\n");
        ApBuf label; ApBufInit(&label);
        ApBufAppend(&label, "{");
        for (size_t i = 0; i < p->subparsers->names.count; ++i) {
            if (i) ApBufAppend(&label, ",");
            ApBufAppend(&label, p->subparsers->names.items[i]);
        }
        ApBufAppend(&label, "}");
        ApHelpRow(&out, label.data, p->subparsers->help);
        ApFree(label.data);
        for (size_t i = 0; i < p->subparsers->names.count; ++i) {
            ApBuf l2; ApBufInit(&l2);
            ApBufAppendf(&l2, "  %s", p->subparsers->names.items[i]);
            ApHelpRow(&out, l2.data, p->subparsers->helps.items[i]);
            ApFree(l2.data);
        }
    }

    bool any_opt = false;
    for (size_t i = 0; i < p->args.count; ++i) if (!p->args.items[i]->is_positional) { any_opt = true; break; }
    if (any_opt) {
        ApBufAppend(&out, "\noptions:\n");
        for (size_t i = 0; i < p->args.count; ++i) {
            Argument *a = p->args.items[i];
            if (a->is_positional) continue;
            ApBuf label; ApBufInit(&label);
            ApRenderArgLabel(a, &label);
            ApHelpRow(&out, label.data ? label.data : a->dest, a->help);
            ApFree(label.data);
        }
    }

    if (p->epilog && p->epilog[0]) {
        ApBufAppend(&out, "\n");
        ApBufAppend(&out, p->epilog);
        ApBufAppend(&out, "\n");
    }

    return out.data;
}

ARGPARSEAPI void PrintUsage(const Parser *p, FILE *out) {
    if (!out) out = stdout;
    char *s = FormatUsage(p);
    fputs(s, out);
    ApFree(s);
}

ARGPARSEAPI void PrintHelp(const Parser *p, FILE *out) {
    if (!out) out = stdout;
    char *s = FormatHelp(p);
    fputs(s, out);
    ApFree(s);
}

/* ===========================================================================
 *  Errors
 * =========================================================================== */

ARGPARSEAPI const char *GetError(const Parser *p) {
    return p ? p->error_msg : NULL;
}

ARGPARSEAPI void Error(Parser *p, const char *fmt, ...) {
    if (!p) return;
    char buf[1024];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    ApFree(p->error_msg);
    p->error_msg = ApStrdup(buf);

    if (p->exit_on_error) {
        char *usage = FormatUsage(p);
        fputs(usage, stderr);
        ApFree(usage);
        fprintf(stderr, "%s: error: %s\n", p->prog ? p->prog : "PROG", buf);
        exit(2);
    }
}


/* ===========================================================================
 *  Parsing logic
 * =========================================================================== */

typedef struct {
    char **argv;
    int    argc;
    int    pos;
} ApArgstream;

static const char *ApPeek(const ApArgstream *s) {
    return (s->pos < s->argc) ? s->argv[s->pos] : NULL;
}

static const char *ApNext(ApArgstream *s) {
    if (s->pos >= s->argc) return NULL;
    return s->argv[s->pos++];
}

static bool ApApplyStore(Argument *a, Namespace *ns,
                            const char *value, Parser *parser) {
    char errbuf[256];
    if (a->choices.count > 0) {
        if (!ApValidateChoice(a, value, errbuf, sizeof(errbuf))) {
            Error(parser, "%s", errbuf);
            return false;
        }
    }
    if (!ApValidateType(a, value, errbuf, sizeof(errbuf))) {
        Error(parser, "%s", errbuf);
        return false;
    }
    ApValue *v = ApNsEnsure(ns, a->dest);
    v->present = true;
    if (a->action == ACTION_APPEND) {
        ApValueAppend(v, value);
    } else {
        ApValueSetScalar(v, value);
    }
    return true;
}

static int ApExpectedCount(const Argument *a) {
    switch (a->action) {
        case ACTION_STORE_TRUE:
        case ACTION_STORE_FALSE:
        case ACTION_STORE_CONST:
        case ACTION_APPEND_CONST:
        case ACTION_HELP:
        case ACTION_VERSION:
        case ACTION_COUNT:
            return 0;
        default: break;
    }
    switch (a->nargs) {
        case NARGS_NONE:         return 1;
        case NARGS_N:            return a->nargs_n;
        case NARGS_OPTIONAL:     return -2;
        case NARGS_ZERO_OR_MORE: return -1;
        case NARGS_ONE_OR_MORE:  return -1;
        case NARGS_REMAINDER:    return -1;
    }
    return 1;
}

static bool ApConsumeValues(Parser *parser, Argument *a,
                               Namespace *ns, ApArgstream *s,
                               const char *first_value) {
    a->seen = true;
    int expected = ApExpectedCount(a);

    if (expected == 0) {
        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = true;
        switch (a->action) {
            case ACTION_STORE_TRUE:  ApValueSetScalar(v, "true"); break;
            case ACTION_STORE_FALSE: ApValueSetScalar(v, "false"); break;
            case ACTION_STORE_CONST: ApValueSetScalar(v, a->const_value ? a->const_value : ""); break;
            case ACTION_APPEND_CONST: ApValueAppend(v, a->const_value ? a->const_value : ""); break;
            case ACTION_HELP: {
                PrintHelp(parser, stdout);
                exit(0);
            }
            case ACTION_VERSION: {
                fprintf(stdout, "%s\n", parser->version ? parser->version : "");
                exit(0);
            }
            case ACTION_COUNT: {
                v->is_counter = true;
                v->counter += 1;
                char buf[32]; snprintf(buf, sizeof(buf), "%ld", v->counter);
                ApValueSetScalar(v, buf);
                break;
            }
            default: break;
        }
        return true;
    }

    int taken = 0;
    bool first_is_provided = (first_value != NULL);

    #define AP_TOK_IS_VALUE(tok) \
        (!ApIsOptionString(parser, (tok)) || \
         (ApLooksNumeric(tok) && ApFindOptionalExact(parser, (tok)) == NULL))

    if (a->nargs == NARGS_REMAINDER) {
        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = true;
        if (first_is_provided) { ApValueAppend(v, first_value); taken++; }
        const char *t;
        while ((t = ApPeek(s)) != NULL) {
            ApValueAppend(v, t);
            (void)ApNext(s); taken++;
        }
        return true;
    }

    if (a->nargs == NARGS_N) {
        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = true;
        v->is_list = true;
        if (first_is_provided) { ApValueAppend(v, first_value); taken++; }
        while (taken < a->nargs_n) {
            const char *t = ApPeek(s);
            if (!t || !AP_TOK_IS_VALUE(t)) {
                Error(parser, "argument %s: expected %d argument%s",
                      a->dest, a->nargs_n, a->nargs_n == 1 ? "" : "s");
                return false;
            }
            char errbuf[256];
            if (a->choices.count > 0 && !ApValidateChoice(a, t, errbuf, sizeof(errbuf))) {
                Error(parser, "%s", errbuf); return false;
            }
            if (!ApValidateType(a, t, errbuf, sizeof(errbuf))) {
                Error(parser, "%s", errbuf); return false;
            }
            ApValueAppend(v, t);
            (void)ApNext(s); taken++;
        }
        return true;
    }

    if (a->nargs == NARGS_OPTIONAL) {
        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = true;
        if (first_is_provided) {
            return ApApplyStore(a, ns, first_value, parser);
        }
        const char *t = ApPeek(s);
        if (t && AP_TOK_IS_VALUE(t)) {
            (void)ApNext(s);
            return ApApplyStore(a, ns, t, parser);
        }
        if (a->const_value && !a->is_positional) ApValueSetScalar(v, a->const_value);
        else if (a->default_value)               ApValueSetScalar(v, a->default_value);
        return true;
    }

    if (a->nargs == NARGS_ZERO_OR_MORE || a->nargs == NARGS_ONE_OR_MORE) {
        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = true;
        v->is_list = true;
        if (first_is_provided) { ApValueAppend(v, first_value); taken++; }
        const char *t;
        while ((t = ApPeek(s)) != NULL && AP_TOK_IS_VALUE(t)) {
            char errbuf[256];
            if (a->choices.count > 0 && !ApValidateChoice(a, t, errbuf, sizeof(errbuf))) {
                Error(parser, "%s", errbuf); return false;
            }
            if (!ApValidateType(a, t, errbuf, sizeof(errbuf))) {
                Error(parser, "%s", errbuf); return false;
            }
            ApValueAppend(v, t);
            (void)ApNext(s); taken++;
        }
        if (a->nargs == NARGS_ONE_OR_MORE && taken < 1) {
            Error(parser, "argument %s: expected at least one argument", a->dest);
            return false;
        }
        return true;
    }

    {
        const char *val = first_value;
        if (!val) {
            const char *t = ApPeek(s);
            if (!t || !AP_TOK_IS_VALUE(t)) {
                Error(parser, "argument %s: expected one argument", a->dest);
                return false;
            }
            val = ApNext(s);
        }
        return ApApplyStore(a, ns, val, parser);
    }

    #undef AP_TOK_IS_VALUE
}

static bool ApAssignPositionals(Parser *parser, Namespace *ns,
                                   Argument **pos, size_t pn,
                                   const char **toks, size_t tn) {
    int *mn = (int *)ApXcalloc(pn, sizeof(int));
    int *mx = (int *)ApXcalloc(pn, sizeof(int));
    for (size_t i = 0; i < pn; ++i) {
        switch (pos[i]->nargs) {
            case NARGS_NONE:         mn[i] = 1; mx[i] = 1; break;
            case NARGS_N:            mn[i] = pos[i]->nargs_n; mx[i] = pos[i]->nargs_n; break;
            case NARGS_OPTIONAL:     mn[i] = 0; mx[i] = 1; break;
            case NARGS_ZERO_OR_MORE: mn[i] = 0; mx[i] = INT_MAX; break;
            case NARGS_ONE_OR_MORE:  mn[i] = 1; mx[i] = INT_MAX; break;
            case NARGS_REMAINDER:    mn[i] = 0; mx[i] = INT_MAX; break;
        }
    }

    int total_min = 0;
    for (size_t i = 0; i < pn; ++i) total_min += mn[i];
    if ((int)tn < total_min) {
        ApBuf list; ApBufInit(&list);
        for (size_t i = 0; i < pn; ++i) {
            if (i) ApBufAppend(&list, ", ");
            ApBufAppend(&list, pos[i]->dest);
        }
        Error(parser, "the following arguments are required: %s", list.data ? list.data : "");
        ApFree(list.data);
        ApFree(mn); ApFree(mx);
        return false;
    }

    int extra = (int)tn - total_min;
    int *take = (int *)ApXcalloc(pn, sizeof(int));
    for (size_t i = 0; i < pn; ++i) take[i] = mn[i];
    for (size_t i = 0; i < pn && extra > 0; ++i) {
        int can = mx[i] - take[i];
        int give = can < extra ? can : extra;
        take[i] += give;
        extra -= give;
    }
    if (extra > 0) {
        Error(parser, "unrecognized arguments: %s", toks[tn - extra]);
        ApFree(mn); ApFree(mx); ApFree(take);
        return false;
    }

    size_t idx = 0;
    for (size_t i = 0; i < pn; ++i) {
        Argument *a = pos[i];
        a->seen = (take[i] > 0);
        if (take[i] == 0) continue;
        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = true;

        if (a->nargs == NARGS_NONE) {
            char errbuf[256];
            if (a->choices.count > 0 && !ApValidateChoice(a, toks[idx], errbuf, sizeof(errbuf))) {
                Error(parser, "%s", errbuf); goto fail;
            }
            if (!ApValidateType(a, toks[idx], errbuf, sizeof(errbuf))) {
                Error(parser, "%s", errbuf); goto fail;
            }
            ApValueSetScalar(v, toks[idx++]);
        } else if (a->nargs == NARGS_OPTIONAL) {
            if (take[i] == 1) {
                char errbuf[256];
                if (a->choices.count > 0 && !ApValidateChoice(a, toks[idx], errbuf, sizeof(errbuf))) {
                    Error(parser, "%s", errbuf); goto fail;
                }
                if (!ApValidateType(a, toks[idx], errbuf, sizeof(errbuf))) {
                    Error(parser, "%s", errbuf); goto fail;
                }
                ApValueSetScalar(v, toks[idx++]);
            }
        } else {
            v->is_list = true;
            for (int k = 0; k < take[i]; ++k) {
                char errbuf[256];
                if (a->choices.count > 0 && !ApValidateChoice(a, toks[idx], errbuf, sizeof(errbuf))) {
                    Error(parser, "%s", errbuf); goto fail;
                }
                if (!ApValidateType(a, toks[idx], errbuf, sizeof(errbuf))) {
                    Error(parser, "%s", errbuf); goto fail;
                }
                ApValueAppend(v, toks[idx++]);
            }
        }
    }

    ApFree(mn); ApFree(mx); ApFree(take);
    return true;
fail:
    ApFree(mn); ApFree(mx); ApFree(take);
    return false;
}


/* ===========================================================================
 *  Main parse loop
 * =========================================================================== */

static Namespace *ApNamespaceNew(void) {
    Namespace *ns = (Namespace *)ApXcalloc(1, sizeof(*ns));
    ApValvecInit(&ns->values);
    return ns;
}

ARGPARSEAPI void FreeNamespace(Namespace *ns) {
    if (!ns) return;
    for (size_t i = 0; i < ns->values.count; ++i) ApValueFree(ns->values.items[i]);
    ApFree(ns->values.items);
    ApFree(ns->subcommand);
    FreeNamespace(ns->sub_namespace);
    ApFree(ns);
}

static void ApApplyDefaults(Parser *p, Namespace *ns) {
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (a->seen) continue;
        if (a->action == ACTION_HELP || a->action == ACTION_VERSION) continue;

        if (a->default_value && strcmp(a->default_value, SUPPRESS) == 0) continue;

        ApValue *v = ApNsEnsure(ns, a->dest);
        v->present = false;

        switch (a->action) {
            case ACTION_STORE_TRUE:
                ApValueSetScalar(v, a->default_value ? a->default_value : "false");
                break;
            case ACTION_STORE_FALSE:
                ApValueSetScalar(v, a->default_value ? a->default_value : "true");
                break;
            case ACTION_COUNT:
                v->is_counter = true;
                v->counter = a->default_value ? strtol(a->default_value, NULL, 10) : 0;
                if (a->default_value) ApValueSetScalar(v, a->default_value);
                else ApValueSetScalar(v, "0");
                break;
            case ACTION_APPEND:
            case ACTION_APPEND_CONST:
                v->is_list = true;
                if (a->default_value) ApValueAppend(v, a->default_value);
                break;
            default:
                if (a->default_value) ApValueSetScalar(v, a->default_value);
                else if (a->nargs == NARGS_ZERO_OR_MORE) v->is_list = true;
                break;
        }
    }
}

static bool ApCheckRequired(Parser *p) {
    ApBuf missing; ApBufInit(&missing);
    bool any = false;
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (!a->required) continue;
        if (a->seen) continue;
        if (any) ApBufAppend(&missing, ", ");
        if (a->is_positional) ApBufAppend(&missing, a->dest);
        else ApBufAppend(&missing, a->option_strings.items[0]);
        any = true;
    }
    if (any) {
        Error(p, "the following arguments are required: %s", missing.data ? missing.data : "");
        ApFree(missing.data);
        return false;
    }
    ApFree(missing.data);
    return true;
}

typedef struct {
    const char **items;
    size_t       count;
    size_t       capacity;
} ApPtrvec;

static void ApPtrvecInit(ApPtrvec *v) { v->items = NULL; v->count = 0; v->capacity = 0; }
static void ApPtrvecPush(ApPtrvec *v, const char *s) {
    if (v->count + 1 > v->capacity) {
        v->capacity = v->capacity ? v->capacity * 2 : 8;
        v->items = (const char **)ApXrealloc((void *)v->items, v->capacity * sizeof(*v->items));
    }
    v->items[v->count++] = s;
}

static Namespace *ApParseImpl(Parser *p, int argc, char **argv,
                                 bool allow_unknown,
                                 ApPtrvec *unknown_out) {
    ApMaybeInjectHelp(p);

    for (size_t i = 0; i < p->args.count; ++i) p->args.items[i]->seen = false;

    Namespace *ns = ApNamespaceNew();

    int start = (argc > 0) ? 1 : 0;

    ApArgstream s;
    s.argv = argv;
    s.argc = argc;
    s.pos  = start;

    ApPtrvec positional_toks; ApPtrvecInit(&positional_toks);

    bool only_positionals = false;
    int subparser_split_at = -1;
    Parser *chosen_sub = NULL;
    const char *chosen_sub_name = NULL;

    int top_pos_min = 0;
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (!a->is_positional) continue;
        switch (a->nargs) {
            case NARGS_NONE:         top_pos_min += 1; break;
            case NARGS_N:            top_pos_min += a->nargs_n; break;
            case NARGS_ONE_OR_MORE:  top_pos_min += 1; break;
            case NARGS_OPTIONAL:
            case NARGS_ZERO_OR_MORE:
            case NARGS_REMAINDER:    break;
        }
    }

    while (s.pos < s.argc) {
        const char *tok = ApNext(&s);
        if (!tok) break;

        if (only_positionals) {
            if (p->subparsers && (int)positional_toks.count >= top_pos_min) {
                bool matched = false;
                for (size_t i = 0; i < p->subparsers->names.count; ++i) {
                    if (strcmp(tok, p->subparsers->names.items[i]) == 0) {
                        chosen_sub = p->subparsers->parsers[i];
                        chosen_sub_name = p->subparsers->names.items[i];
                        subparser_split_at = s.pos;
                        matched = true;
                        break;
                    }
                }
                if (matched) break;
            }
            ApPtrvecPush(&positional_toks, tok);
            continue;
        }

        if (strcmp(tok, "--") == 0) { only_positionals = true; continue; }

        if (ApIsOptionString(p, tok)) {
            if (ApLooksNumeric(tok) && !ApFindOptionalExact(p, tok)) {
                ApPtrvecPush(&positional_toks, tok);
                continue;
            }

            const char *eq = NULL;
            char *opt_name = NULL;
            const char *embed_value = NULL;
            const char *cluster_rest = NULL;
            if (ApIsLongOption(p, tok)) {
                eq = strchr(tok, '=');
                if (eq) {
                    size_t L = (size_t)(eq - tok);
                    opt_name = ApStrdupN(tok, L);
                    embed_value = eq + 1;
                } else {
                    opt_name = ApStrdup(tok);
                }
            } else {
                opt_name = ApStrdupN(tok, 2);
                if (tok[2] != '\0') cluster_rest = tok + 2;
            }

            Argument *a = ApFindOptionalExact(p, opt_name);
            if (!a && ApIsLongOption(p, opt_name) && p->allow_abbrev) {
                bool ambig = false;
                char ambbuf[256] = {0};
                a = ApFindOptionalAbbrev(p, opt_name, &ambig, ambbuf, sizeof(ambbuf));
                if (ambig) {
                    Error(p, "ambiguous option: %s could match %s", opt_name, ambbuf);
                    ApFree(opt_name);
                    if (allow_unknown && unknown_out) ApPtrvecPush(unknown_out, tok);
                    if (!p->exit_on_error) { FreeNamespace(ns); ApFree(positional_toks.items); return NULL; }
                    continue;
                }
            }

            if (!a) {
                if (allow_unknown && unknown_out) {
                    ApPtrvecPush(unknown_out, tok);
                    ApFree(opt_name);
                    continue;
                }
                Error(p, "unrecognized arguments: %s", tok);
                ApFree(opt_name);
                ApFree(positional_toks.items);
                if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
                continue;
            }

            ApFree(opt_name);

            int exp = ApExpectedCount(a);

            if (cluster_rest) {
                if (exp != 0) {
                    if (!ApConsumeValues(p, a, ns, &s, cluster_rest)) {
                        ApFree(positional_toks.items);
                        if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
                    }
                    continue;
                }
                if (!ApConsumeValues(p, a, ns, &s, NULL)) {
                    ApFree(positional_toks.items);
                    if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
                }
                bool cluster_failed = false;
                size_t i = 0;
                while (cluster_rest[i]) {
                    char short_opt[3] = { tok[0], cluster_rest[i], 0 };
                    Argument *sa = ApFindOptionalExact(p, short_opt);
                    if (!sa) {
                        Error(p, "unrecognized arguments: %s", short_opt);
                        cluster_failed = true; break;
                    }
                    int sexp = ApExpectedCount(sa);
                    if (sexp == 0) {
                        if (!ApConsumeValues(p, sa, ns, &s, NULL)) {
                            cluster_failed = true; break;
                        }
                        i++;
                    } else {
                        const char *val = (cluster_rest[i+1] != 0) ? (cluster_rest + i + 1) : NULL;
                        if (!ApConsumeValues(p, sa, ns, &s, val)) {
                            cluster_failed = true; break;
                        }
                        break;
                    }
                }
                if (cluster_failed) {
                    ApFree(positional_toks.items);
                    if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
                }
                continue;
            }

            if (exp == 0 && embed_value) {
                if (a->action == ACTION_STORE_TRUE || a->action == ACTION_STORE_FALSE) {
                    if (!ApApplyStore(a, ns, embed_value, p)) {
                        if (!p->exit_on_error) { FreeNamespace(ns); ApFree(positional_toks.items); return NULL; }
                    }
                    a->seen = true;
                    continue;
                }
                Error(p, "argument %s: ignored explicit argument '%s'",
                      a->option_strings.items[0], embed_value);
                if (!p->exit_on_error) { FreeNamespace(ns); ApFree(positional_toks.items); return NULL; }
                continue;
            }

            if (!ApConsumeValues(p, a, ns, &s, embed_value)) {
                ApFree(positional_toks.items);
                if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
            }
            continue;
        }

        if (p->subparsers && (int)positional_toks.count >= top_pos_min) {
            for (size_t i = 0; i < p->subparsers->names.count; ++i) {
                if (strcmp(tok, p->subparsers->names.items[i]) == 0) {
                    chosen_sub = p->subparsers->parsers[i];
                    chosen_sub_name = p->subparsers->names.items[i];
                    subparser_split_at = s.pos;
                    break;
                }
            }
            if (chosen_sub) break;
        }

        ApPtrvecPush(&positional_toks, tok);
    }

    Argument **pos_args = NULL;
    size_t pos_n = 0, pos_cap = 0;
    for (size_t i = 0; i < p->args.count; ++i) {
        Argument *a = p->args.items[i];
        if (!a->is_positional) continue;
        if (pos_n + 1 > pos_cap) {
            pos_cap = pos_cap ? pos_cap * 2 : 4;
            pos_args = (Argument **)ApXrealloc(pos_args, pos_cap * sizeof(*pos_args));
        }
        pos_args[pos_n++] = a;
    }

    if (pos_n > 0 && positional_toks.count > 0) {
        if (!ApAssignPositionals(p, ns, pos_args, pos_n,
                                    positional_toks.items, positional_toks.count)) {
            ApFree(pos_args);
            ApFree(positional_toks.items);
            if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
        }
    } else if (pos_n == 0 && positional_toks.count > 0) {
        if (allow_unknown && unknown_out) {
            for (size_t i = 0; i < positional_toks.count; ++i)
                ApPtrvecPush(unknown_out, positional_toks.items[i]);
        } else {
            Error(p, "unrecognized arguments: %s", positional_toks.items[0]);
            ApFree(pos_args);
            ApFree(positional_toks.items);
            if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
        }
    }
    ApFree(pos_args);
    ApFree(positional_toks.items);

    ApApplyDefaults(p, ns);

    if (p->subparsers) {
        if (chosen_sub) {
            int n_left = s.argc - subparser_split_at;
            char **sub_argv = NULL;
            if (n_left > 0) {
                sub_argv = (char **)ApXmalloc(((size_t)n_left + 1) * sizeof(char *));
                sub_argv[0] = (char *)chosen_sub_name;
                for (int i = 0; i < n_left; ++i) sub_argv[i+1] = s.argv[subparser_split_at + i];
            } else {
                sub_argv = (char **)ApXmalloc(2 * sizeof(char *));
                sub_argv[0] = (char *)chosen_sub_name;
                sub_argv[1] = NULL;
            }
            Namespace *sub_ns = ParseArgs(chosen_sub, n_left + 1, sub_argv);
            ApFree(sub_argv);
            if (!sub_ns) { FreeNamespace(ns); return NULL; }

            ns->subcommand    = ApStrdup(chosen_sub_name);
            ns->sub_namespace = sub_ns;
            ApValue *sv = ApNsEnsure(ns, p->subparsers->dest);
            sv->present = true;
            ApValueSetScalar(sv, chosen_sub_name);

            for (size_t i = 0; i < sub_ns->values.count; ++i) {
                ApValue *src = sub_ns->values.items[i];
                if (ApNsFind(ns, src->name)) continue;
                ApValue *dst = ApNsEnsure(ns, src->name);
                dst->present = src->present;
                dst->is_list = src->is_list;
                dst->is_counter = src->is_counter;
                dst->counter = src->counter;
                if (src->scalar) ApValueSetScalar(dst, src->scalar);
                for (size_t k = 0; k < src->list.count; ++k)
                    ApValueAppend(dst, src->list.items[k]);
            }
        } else if (p->subparsers->required) {
            Error(p, "the following arguments are required: %s", p->subparsers->dest);
            if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
        }
    }

    if (!ApCheckRequired(p)) {
        if (!p->exit_on_error) { FreeNamespace(ns); return NULL; }
    }

    return ns;
}

ARGPARSEAPI Namespace *ParseArgs(Parser *p, int argc, char **argv) {
    return ApParseImpl(p, argc, argv, false, NULL);
}

ARGPARSEAPI Namespace *ParseKnownArgs(Parser *p, int argc, char **argv,
                                      int *out_unknown_count, char ***out_unknown) {
    ApPtrvec unk; ApPtrvecInit(&unk);
    Namespace *ns = ApParseImpl(p, argc, argv, true, &unk);
    if (out_unknown_count) *out_unknown_count = (int)unk.count;
    if (out_unknown) {
        if (unk.count == 0) { *out_unknown = NULL; }
        else {
            char **arr = (char **)ApXmalloc((unk.count + 1) * sizeof(char *));
            for (size_t i = 0; i < unk.count; ++i) arr[i] = (char *)unk.items[i];
            arr[unk.count] = NULL;
            *out_unknown = arr;
        }
    }
    ApFree(unk.items);
    return ns;
}

/* ===========================================================================
 *  Namespace accessors
 * =========================================================================== */

ARGPARSEAPI bool Has(const Namespace *ns, const char *name) {
    return ApNsFind(ns, name) != NULL;
}

ARGPARSEAPI bool WasPresent(const Namespace *ns, const char *name) {
    const ApValue *v = ApNsFind(ns, name);
    return v && v->present;
}

ARGPARSEAPI const char *GetString(const Namespace *ns, const char *name) {
    const ApValue *v = ApNsFind(ns, name);
    if (!v) return NULL;
    if (v->scalar) return v->scalar;
    if (v->is_list && v->list.count > 0) return v->list.items[0];
    return NULL;
}

ARGPARSEAPI long GetInt(const Namespace *ns, const char *name) {
    const char *s = GetString(ns, name);
    if (!s) return 0;
    return strtol(s, NULL, 10);
}

ARGPARSEAPI double GetFloat(const Namespace *ns, const char *name) {
    const char *s = GetString(ns, name);
    if (!s) return 0.0;
    return strtod(s, NULL);
}

ARGPARSEAPI bool GetBool(const Namespace *ns, const char *name) {
    const ApValue *v = ApNsFind(ns, name);
    if (!v) return false;
    if (v->is_counter) return v->counter != 0;
    return ApStrToBool(v->scalar);
}

ARGPARSEAPI size_t GetCount(const Namespace *ns, const char *name) {
    const ApValue *v = ApNsFind(ns, name);
    if (!v) return 0;
    if (v->is_counter) return (size_t)(v->counter > 0 ? v->counter : 0);
    if (v->is_list) return v->list.count;
    if (v->scalar) return 1;
    return 0;
}

ARGPARSEAPI const char *GetStringAt(const Namespace *ns, const char *name, size_t idx) {
    const ApValue *v = ApNsFind(ns, name);
    if (!v) return NULL;
    if (v->is_list) return idx < v->list.count ? v->list.items[idx] : NULL;
    if (idx == 0) return v->scalar;
    return NULL;
}

ARGPARSEAPI long GetIntAt(const Namespace *ns, const char *name, size_t idx) {
    const char *s = GetStringAt(ns, name, idx);
    return s ? strtol(s, NULL, 10) : 0;
}

ARGPARSEAPI double GetFloatAt(const Namespace *ns, const char *name, size_t idx) {
    const char *s = GetStringAt(ns, name, idx);
    return s ? strtod(s, NULL) : 0.0;
}

ARGPARSEAPI const char *GetSubcommand(const Namespace *ns) {
    return ns ? ns->subcommand : NULL;
}

ARGPARSEAPI const Namespace *GetSubnamespace(const Namespace *ns) {
    return ns ? ns->sub_namespace : NULL;
}

#endif /* ARGPARSE_IMPLEMENTATION_INCLUDED_ */
#endif /* ARGPARSE_IMPLEMENTATION */
