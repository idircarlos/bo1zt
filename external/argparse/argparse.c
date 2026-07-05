// Single translation unit that pulls in the argparse implementation.
// Compiled as C (see Makefile) so it is not subject to C++'s stricter rules.
#define ARGPARSE_IMPLEMENTATION
#include "argparse.h"
