# argparse.h

A single-header C library for parsing command-line arguments, modelled after
Python's [`argparse`](https://docs.python.org/3/library/argparse.html) module.

It aims to give C programmers the same ergonomic feel as Python's argparse:
declarative argument definition, automatic `--help` generation, type checking,
choices, nargs, subcommands, and friendly error messages, all in a single
header of C99 code.

## Features

- Single-header, drop-in: just include `src/argparse.h`.
- Positional and optional arguments.
- Short (`-v`), long (`--verbose`), short-with-attached-value (`-cVALUE`),
  long-with-equals (`--count=42`), and clustered short flags (`-abc`).
- Long-option abbreviations (`--verb` for `--verbose`) with ambiguity detection.
- Actions: store, store_true, store_false, store_const, append, append_const,
  count, help, version.
- nargs: a fixed `N`, `?` (optional), `*` (zero-or-more), `+` (one-or-more),
  and `REMAINDER`.
- Types: string, int, float, bool, with validation.
- `Choices` enforcement.
- Subcommands via `AddSubparsers` / `AddParser`.
- `ParseKnownArgs` for tools that pass through unknown options.
- The `--` separator stops option parsing.
- Auto-injected `-h, --help` and optional `-V, --version` actions.
- Auto-generated usage and help text in argparse's familiar style.

## Quick start

```c
#define ARGPARSE_IMPLEMENTATION
#include "argparse.h"

int main(int argc, char **argv) {
    Parser *p = NewParser("greet");
    SetDescription(p, "Friendly greeter.");

    AddArgument(p, "name", NULL);                          /* positional */
    ArgAction(AddArgument(p, "-v", "--verbose", NULL),     /* boolean flag */
              ACTION_STORE_TRUE);
    ArgDefault(
        ArgType(AddArgument(p, "-c", "--count", NULL),     /* typed option */
                TYPE_INT),
        "1");

    Namespace *ns = ParseArgs(p, argc, argv);

    long n = GetInt(ns, "count");
    for (long i = 0; i < n; ++i)
        printf("hello, %s%s\n",
               GetString(ns, "name"),
               GetBool(ns, "verbose") ? "!" : "");

    FreeNamespace(ns);
    FreeParser(p);
}
```

```
$ ./greet --help
usage: greet [-h] [-v] [-c COUNT] name

Friendly greeter.

positional arguments:
  name

options:
  -h, --help            show this help message and exit
  -v, --verbose
  -c COUNT, --count COUNT

$ ./greet world -v -c 2
hello, world!
hello, world!
```

## Build

The library is single-header. To use it, define `ARGPARSE_IMPLEMENTATION` in
exactly **one** translation unit before including the header. Other files just
include the header normally.

```sh
make all      # build the example and run the tests
make main     # build the example only
make test     # build and run the tests
make clean
```

Tested with GCC 14 on Windows; only depends on the C99 standard library.

## API at a glance

### Naming conventions

- **Public functions** use `CamelCase` with verb-first naming
  (`NewParser`, `AddArgument`, `FreeNamespace`).
- **Public types** use `CamelCase` (`Parser`, `Argument`, `Namespace`,
  `Subparsers`, `ValueKind`, `ActionKind`, `NargsKind`).
- **Public enum values and constants** use `UPPER_SNAKE_CASE`
  (`TYPE_INT`, `ACTION_STORE_TRUE`, `NARGS_ONE_OR_MORE`, `SUPPRESS`).
- **Argument configuration helpers** are prefixed with `Arg`
  (`ArgHelp`, `ArgType`, `ArgAction`, ...) to avoid clashes with user code.

### Parser

```c
Parser *p = NewParser("prog");
SetDescription(p, "...");
SetEpilog(p, "...");
SetVersion(p, "1.2.3");
SetExitOnError(p, false);   /* return NULL instead of exit(2) */
SetAllowAbbrev(p, false);   /* disable --abbrev for long options */
```

### Adding arguments

```c
AddArgument(p, "filename", NULL);                    /* positional */
AddArgument(p, "-v", "--verbose", NULL);             /* optional */
```

Configuration is chainable; each helper returns the same argument pointer:

```c
ArgHelp    (a, "help text");
ArgDefault (a, "value");
ArgType    (a, TYPE_INT);
ArgAction  (a, ACTION_STORE_TRUE);
ArgNargs   (a, NARGS_ONE_OR_MORE);
ArgNargsN  (a, 3);
ArgRequired(a, true);
ArgChoices (a, "fast", "slow", NULL);
ArgMetavar (a, "FILE");
ArgDest    (a, "my_dest");
ArgConst   (a, "magic");
```

### Subparsers (subcommands)

```c
Subparsers *sp = AddSubparsers(p, "command");
SetSubparsersRequired(sp, true);

Parser *commit = AddParser(sp, "commit", "record changes");
AddArgument(commit, "-m", "--message", NULL);

Parser *push = AddParser(sp, "push", "publish changes");
```

### Parsing

```c
Namespace *ns = ParseArgs(p, argc, argv);

/* or, to silently allow unknown args: */
int n; char **unknown;
ns = ParseKnownArgs(p, argc, argv, &n, &unknown);
```

### Reading values

```c
const char *file = GetString(ns, "filename");
long count       = GetInt   (ns, "count");
bool verbose     = GetBool  (ns, "verbose");

size_t n = GetCount(ns, "items");
for (size_t i = 0; i < n; ++i)
    puts(GetStringAt(ns, "items", i));

const char *cmd = GetSubcommand(ns);
const Namespace *sub = GetSubnamespace(ns);
```

Use `WasPresent(ns, "name")` to distinguish "user typed it" from "default
was used".

### Cleanup

```c
FreeNamespace(ns);
FreeParser(p);
```

## License

See [LICENSE](LICENSE).
