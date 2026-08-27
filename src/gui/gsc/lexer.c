#include "gui/gsc/lexer.h"

#include <Scintilla.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Scintilla colours are 0xBBGGRR, not 0xRRGGBB
#define GSC_COLOR_COMMENT 0x008000
#define GSC_COLOR_NUMBER 0xA06E00
#define GSC_COLOR_STRING 0x1515A3
#define GSC_COLOR_KEYWORD 0xC00000
#define GSC_COLOR_DIRECTIVE 0x800080
#define GSC_COLOR_OPERATOR 0x808080
#define GSC_COLOR_FUNCTION 0x8C3E9B

static const char *const GSC_KEYWORD_TABLE[] = {
    "break", "case", "continue", "default", "do", "else", "for", "foreach", "if", "in",
    "return", "switch", "while",
    "thread", "wait", "waittill", "waittillframeend", "waittillmatch", "endon", "notify",
    "breakpoint", "assert", "include",
    "self", "level", "game", "anim",
    "true", "false", "undefined"
};

#define GSC_KEYWORD_COUNT (sizeof(GSC_KEYWORD_TABLE) / sizeof(GSC_KEYWORD_TABLE[0]))

static bool isIdentifierStart(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool isIdentifierChar(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool isKeyword(const char *text, size_t length) {
    for (size_t i = 0; i < GSC_KEYWORD_COUNT; i++) {
        const char *keyword = GSC_KEYWORD_TABLE[i];
        if (strlen(keyword) == length && strncmp(keyword, text, length) == 0) return true;
    }
    return false;
}

static void setColor(uiScintilla *editor, GscStyle style, int color) {
    uiScintillaSend(editor, SCI_STYLESETFORE, style, color);
}

void gscLexerConfigure(uiScintilla *editor) {
    setColor(editor, GSC_STYLE_COMMENT, GSC_COLOR_COMMENT);
    setColor(editor, GSC_STYLE_NUMBER, GSC_COLOR_NUMBER);
    setColor(editor, GSC_STYLE_STRING, GSC_COLOR_STRING);
    setColor(editor, GSC_STYLE_KEYWORD, GSC_COLOR_KEYWORD);
    setColor(editor, GSC_STYLE_DIRECTIVE, GSC_COLOR_DIRECTIVE);
    setColor(editor, GSC_STYLE_OPERATOR, GSC_COLOR_OPERATOR);
    setColor(editor, GSC_STYLE_FUNCTION, GSC_COLOR_FUNCTION);
    uiScintillaSend(editor, SCI_STYLESETBOLD, GSC_STYLE_KEYWORD, 1);
}

static size_t scanComment(const char *text, size_t length, size_t at) {
    if (text[at + 1] == '/') {
        while (at < length && text[at] != '\n') at++;
        return at;
    }

    at += 2;
    while (at < length && !(text[at] == '*' && text[at + 1] == '/')) at++;
    return at < length ? at + 2 : length;
}

static size_t scanString(const char *text, size_t length, size_t at) {
    at++;
    while (at < length && text[at] != '"' && text[at] != '\n') {
        if (text[at] == '\\' && at + 1 < length) at++;
        at++;
    }
    return at < length && text[at] == '"' ? at + 1 : at;
}

static bool opensCall(const char *text, size_t length, size_t at) {
    while (at < length && (text[at] == ' ' || text[at] == '\t')) at++;
    return at < length && text[at] == '(';
}

void gscLexerStyle(uiScintilla *editor) {
    size_t length = (size_t)uiScintillaSend(editor, SCI_GETLENGTH, 0, 0);

    char *text = (char *)malloc(length + 1);
    if (!text) return;
    uiScintillaSend(editor, SCI_GETTEXT, length + 1, (intptr_t)text);

    uiScintillaSend(editor, SCI_STARTSTYLING, 0, 0);

    size_t at = 0;
    while (at < length) {
        size_t start = at;
        GscStyle style = GSC_STYLE_DEFAULT;
        char c = text[at];

        if (c == '/' && (text[at + 1] == '/' || text[at + 1] == '*')) {
            at = scanComment(text, length, at);
            style = GSC_STYLE_COMMENT;
        } else if (c == '"') {
            at = scanString(text, length, at);
            style = GSC_STYLE_STRING;
        } else if (c == '#') {
            at++;
            while (at < length && isIdentifierChar(text[at])) at++;
            style = GSC_STYLE_DIRECTIVE;
        } else if (isdigit((unsigned char)c)) {
            while (at < length && (isalnum((unsigned char)text[at]) || text[at] == '.')) at++;
            style = GSC_STYLE_NUMBER;
        } else if (isIdentifierStart(c)) {
            while (at < length && isIdentifierChar(text[at])) at++;
            if (isKeyword(text + start, at - start)) style = GSC_STYLE_KEYWORD;
            else if (opensCall(text, length, at)) style = GSC_STYLE_FUNCTION;
        } else if (isspace((unsigned char)c)) {
            while (at < length && isspace((unsigned char)text[at])) at++;
        } else if (strchr("+-*/%=!<>&|^~?:;,.()[]{}", c)) {
            at++;
            style = GSC_STYLE_OPERATOR;
        } else {
            at++;
        }

        uiScintillaSend(editor, SCI_SETSTYLING, at - start, style);
    }

    free(text);
}
