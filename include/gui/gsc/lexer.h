#ifndef GUI_GSC_LEXER_H_
#define GUI_GSC_LEXER_H_

#include <ui.h>

typedef enum {
    GSC_STYLE_DEFAULT = 0,
    GSC_STYLE_COMMENT,
    GSC_STYLE_NUMBER,
    GSC_STYLE_STRING,
    GSC_STYLE_KEYWORD,
    GSC_STYLE_DIRECTIVE,
    GSC_STYLE_OPERATOR,
    GSC_STYLE_FUNCTION
} GscStyle;

void gscLexerConfigure(uiScintilla *editor);
void gscLexerStyle(uiScintilla *editor);

#endif // GUI_GSC_LEXER_H_
