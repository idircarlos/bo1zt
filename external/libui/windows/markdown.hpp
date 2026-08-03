#ifndef __LIBUI_MARKDOWN_HPP__
#define __LIBUI_MARKDOWN_HPP__

#include <string>
#include <vector>

enum {
	markdownStyleBold = 1 << 0,
	markdownStyleItalic = 1 << 1,
	markdownStyleUnderline = 1 << 2,
	markdownStyleCode = 1 << 3,
	markdownStyleLink = 1 << 4,
};

#define markdownStyleCount 32

enum markdownBlockKind {
	markdownParagraph,
	markdownBullet,
	markdownNumber,
	markdownHeading1,
	markdownHeading2,
	markdownHeading3,
	markdownHeading4,
	markdownBlockKindCount,
};

struct markdownRun {
	std::wstring text;
	std::wstring url;
	int style;
};

struct markdownBlock {
	markdownBlockKind kind;
	std::wstring marker;
	int indent;
	std::vector<markdownRun> runs;
};

// markdown.cpp
extern void uiprivMarkdownParse(const char *markdown, std::vector<markdownBlock> *blocks);

#endif
