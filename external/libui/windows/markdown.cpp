// markdown parsing for uiMarkdownViewer
#include "uipriv_windows.hpp"
#include "markdown.hpp"

static void addRun(std::vector<markdownRun> *runs, const std::wstring &text, int style)
{
	markdownRun run;

	if (text.empty())
		return;
	run.text = text;
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

static void parseRuns(const std::wstring &text, std::vector<markdownRun> *runs)
{
	std::wstring current;
	int style = 0;
	size_t i = 0;

	while (i < text.length()) {
		size_t markerLength;
		int toggle = styleMarkerAt(text, i, &markerLength);

		if ((style & markdownStyleCode) != 0 && toggle != markdownStyleCode)
			toggle = 0;

		if (toggle == 0) {
			current += text[i];
			i++;
			continue;
		}

		addRun(runs, current, style);
		current.clear();
		style ^= toggle;
		i += markerLength;
	}
	addRun(runs, current, style);
}

static void addBlock(std::vector<markdownBlock> *blocks, markdownBlockKind kind, const std::wstring &text)
{
	markdownBlock block;

	block.kind = kind;
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

void uiprivMarkdownParse(const char *markdown, std::vector<markdownBlock> *blocks)
{
	WCHAR *wmarkdown = toUTF16(markdown);
	const WCHAR *p = wmarkdown;
	std::wstring pending;

	blocks->clear();
	while (*p != L'\0') {
		std::wstring line;
		size_t markerLength = 0;

		while (*p != L'\0' && *p != L'\n')
			line += *p++;
		if (*p == L'\n')
			p++;
		line = trimmed(line);

		markdownBlockKind heading = headingKindOf(line, &markerLength);
		BOOL bullet = isBullet(line);

		if (line.empty() || markerLength > 0 || bullet) {
			addBlock(blocks, markdownParagraph, pending);
			pending.clear();
		}

		if (line.empty())
			continue;
		if (markerLength > 0) {
			addBlock(blocks, heading, trimmed(line.substr(markerLength)));
			continue;
		}
		if (bullet) {
			addBlock(blocks, markdownBullet, trimmed(line.substr(1)));
			continue;
		}

		if (!pending.empty())
			pending += L' ';
		pending += line;
	}
	addBlock(blocks, markdownParagraph, pending);

	uiprivFree(wmarkdown);
}
