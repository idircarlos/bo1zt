// markdown parsing for uiMarkdownViewer
#include "uipriv_windows.hpp"
#include "markdown.hpp"

static void addRun(std::vector<markdownRun> *runs, const std::wstring &text, int style, const std::wstring &url)
{
	markdownRun run;

	if (text.empty())
		return;
	run.text = text;
	run.url = url;
	run.style = style;
	runs->push_back(run);
}

static int styleMarkerAt(const std::wstring &text, size_t i, size_t *markerLength)
{
	WCHAR c = text[i];
	WCHAR next = (i + 1 < text.length()) ? text[i + 1] : L'\0';

	*markerLength = 1;
	if (c == L'`')
		return markdownStyleCode;
	if (c == L'*' && next == L'*') {
		*markerLength = 2;
		return markdownStyleBold;
	}
	if (c == L'_' && next == L'_') {
		*markerLength = 2;
		return markdownStyleUnderline;
	}
	if (c == L'*' || c == L'_')
		return markdownStyleItalic;
	return 0;
}

static BOOL linkAt(const std::wstring &text, size_t i, std::wstring *label, std::wstring *url, size_t *length)
{
	size_t close, end;

	if (text[i] != L'[')
		return FALSE;
	close = text.find(L']', i + 1);
	if (close == std::wstring::npos || close + 1 >= text.length() || text[close + 1] != L'(')
		return FALSE;
	end = text.find(L')', close + 2);
	if (end == std::wstring::npos)
		return FALSE;

	*label = text.substr(i + 1, close - i - 1);
	*url = text.substr(close + 2, end - close - 2);
	*length = end - i + 1;
	return TRUE;
}

static void parseRuns(const std::wstring &text, std::vector<markdownRun> *runs)
{
	std::wstring current;
	int style = 0;
	size_t i = 0;

	while (i < text.length()) {
		size_t markerLength;
		int toggle = styleMarkerAt(text, i, &markerLength);
		std::wstring label, url;
		size_t linkLength;

		if ((style & markdownStyleCode) != 0 && toggle != markdownStyleCode)
			toggle = 0;

		if (toggle == 0 && (style & markdownStyleCode) == 0 && linkAt(text, i, &label, &url, &linkLength)) {
			addRun(runs, current, style, L"");
			current.clear();
			addRun(runs, label, style | markdownStyleLink, url);
			i += linkLength;
			continue;
		}

		if (toggle == 0) {
			current += text[i];
			i++;
			continue;
		}

		addRun(runs, current, style, L"");
		current.clear();
		style ^= toggle;
		i += markerLength;
	}
	addRun(runs, current, style, L"");
}

static void addBlock(std::vector<markdownBlock> *blocks, markdownBlockKind kind, int indent, const std::wstring &marker, const std::wstring &text)
{
	markdownBlock block;

	block.kind = kind;
	block.marker = marker;
	block.indent = indent;
	parseRuns(text, &(block.runs));
	if (block.runs.empty())
		return;
	blocks->push_back(block);
}

static std::wstring trimmed(const std::wstring &line)
{
	size_t start = line.find_first_not_of(L" \t");
	size_t end = line.find_last_not_of(L" \t\r");

	if (start == std::wstring::npos)
		return std::wstring();
	return line.substr(start, end - start + 1);
}

static markdownBlockKind headingKindOf(const std::wstring &line, size_t *markerLength)
{
	size_t hashes = 0;

	while (hashes < line.length() && line[hashes] == L'#')
		hashes++;
	if (hashes == 0 || hashes > 4)
		return markdownParagraph;
	if (hashes >= line.length() || line[hashes] != L' ')
		return markdownParagraph;

	*markerLength = hashes;
	return (markdownBlockKind) (markdownHeading1 + (hashes - 1));
}

static BOOL isBullet(const std::wstring &line)
{
	if (line.length() < 2)
		return FALSE;
	if (line[0] != L'-' && line[0] != L'*' && line[0] != L'+')
		return FALSE;
	return line[1] == L' ';
}

static BOOL numberMarkerOf(const std::wstring &line, std::wstring *marker)
{
	size_t digits = 0;

	while (digits < line.length() && iswdigit(line[digits]))
		digits++;
	if (digits == 0 || digits + 1 >= line.length())
		return FALSE;
	if (line[digits] != L'.' && line[digits] != L')')
		return FALSE;
	if (line[digits + 1] != L' ')
		return FALSE;

	*marker = line.substr(0, digits + 1);
	return TRUE;
}

// a tab nests one level, as does every group of four spaces; a leftover group of
// at least two counts as well, so bullets aligned under a numbered item nest too
static int indentLevelOf(const std::wstring &line, size_t *offset)
{
	int level = 0;
	int spaces = 0;
	size_t i = 0;

	while (i < line.length() && (line[i] == L' ' || line[i] == L'\t')) {
		if (line[i] == L'\t') {
			level++;
			spaces = 0;
		} else
			spaces++;
		i++;
	}
	*offset = i;
	return level + (spaces + 2) / 4;
}

struct pendingBlock {
	markdownBlockKind kind;
	std::wstring marker;
	std::wstring text;
	int indent;
};

static void beginPending(pendingBlock *pending, markdownBlockKind kind, int indent, const std::wstring &marker, const std::wstring &text)
{
	pending->kind = kind;
	pending->marker = marker;
	pending->text = text;
	pending->indent = indent;
}

static void flushPending(std::vector<markdownBlock> *blocks, pendingBlock *pending)
{
	addBlock(blocks, pending->kind, pending->indent, pending->marker, pending->text);
	beginPending(pending, markdownParagraph, 0, L"", L"");
}

void uiprivMarkdownParse(const char *markdown, std::vector<markdownBlock> *blocks)
{
	WCHAR *wmarkdown = toUTF16(markdown);
	const WCHAR *p = wmarkdown;
	pendingBlock pending;

	blocks->clear();
	beginPending(&pending, markdownParagraph, 0, L"", L"");
	while (*p != L'\0') {
		std::wstring raw, line, marker;
		size_t markerLength = 0, offset;
		int indent;

		while (*p != L'\0' && *p != L'\n')
			raw += *p++;
		if (*p == L'\n')
			p++;
		indent = indentLevelOf(raw, &offset);
		line = trimmed(raw.substr(offset));

		markdownBlockKind heading = headingKindOf(line, &markerLength);

		if (line.empty()) {
			flushPending(blocks, &pending);
			continue;
		}
		if (markerLength > 0) {
			flushPending(blocks, &pending);
			addBlock(blocks, heading, indent, L"", trimmed(line.substr(markerLength)));
			continue;
		}
		if (isBullet(line)) {
			flushPending(blocks, &pending);
			beginPending(&pending, markdownBullet, indent, L"\x2022", trimmed(line.substr(1)));
			continue;
		}
		if (numberMarkerOf(line, &marker)) {
			flushPending(blocks, &pending);
			beginPending(&pending, markdownNumber, indent, marker, trimmed(line.substr(marker.length())));
			continue;
		}

		// an unmarked line continues the block above it, list item included
		if (pending.text.empty())
			beginPending(&pending, markdownParagraph, indent, L"", line);
		else
			pending.text += L' ' + line;
	}
	flushPending(blocks, &pending);

	uiprivFree(wmarkdown);
}
