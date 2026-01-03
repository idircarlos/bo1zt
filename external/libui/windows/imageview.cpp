// uiImageView - A control that displays an image and can be added to containers
#include "uipriv_windows.hpp"

struct uiImageView {
	uiWindowsControl c;
	HWND hwnd;
	int width;
	int height;
	HBITMAP hBitmap;
	IWICBitmap *wicBitmap;
};

static void uiImageViewDraw(uiImageView *iv, HDC hdc)
{
	if (iv->hBitmap == NULL)
		return;

	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap = SelectObject(hdcMem, iv->hBitmap);

	BITMAP bm;
	GetObject(iv->hBitmap, sizeof(bm), &bm);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;

	AlphaBlend(hdc, 0, 0, iv->width, iv->height,
		hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, bf);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
}

static LRESULT CALLBACK imageViewSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiImageView *iv = (uiImageView *)dwRefData;

	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			uiImageViewDraw(iv, hdc);
			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, imageViewSubProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void uiImageViewDestroy(uiControl *c)
{
	uiImageView *iv = uiImageView(c);

	if (iv->hBitmap != NULL)
		DeleteObject(iv->hBitmap);
	if (iv->wicBitmap != NULL)
		iv->wicBitmap->Release();

	uiWindowsEnsureDestroyWindow(iv->hwnd);
	uiFreeControl(c);
}

uiWindowsControlAllDefaultsExceptDestroy(uiImageView)

static void uiImageViewMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiImageView *iv = uiImageView(c);
	*width = iv->width;
	*height = iv->height;
}

static void uiImageViewUpdateBitmap(uiImageView *iv)
{
	if (iv->wicBitmap == NULL)
		return;

	if (iv->hBitmap != NULL) {
		DeleteObject(iv->hBitmap);
		iv->hBitmap = NULL;
	}

	HDC hdc = GetDC(iv->hwnd);
	HRESULT hr = uiprivWICToGDI(iv->wicBitmap, hdc, iv->width, iv->height, &iv->hBitmap);
	ReleaseDC(iv->hwnd, hdc);

	if (FAILED(hr))
		iv->hBitmap = NULL;

	InvalidateRect(iv->hwnd, NULL, TRUE);
}

uiImageView *uiNewImageView(int width, int height)
{
	uiImageView *iv;

	uiWindowsNewControl(uiImageView, iv);

	iv->width = width;
	iv->height = height;
	iv->hBitmap = NULL;
	iv->wicBitmap = NULL;

	iv->hwnd = uiWindowsEnsureCreateControlHWND(0,
		L"static", L"",
		SS_OWNERDRAW,
		hInstance, NULL,
		FALSE);

	SetWindowSubclass(iv->hwnd, imageViewSubProc, 0, (DWORD_PTR)iv);

	return iv;
}

int uiImageViewSetFromData(uiImageView *iv, const void *data, size_t size)
{
	if (iv->wicBitmap != NULL) {
		iv->wicBitmap->Release();
		iv->wicBitmap = NULL;
	}

	IStream *stream = SHCreateMemStream((const BYTE *)data, (UINT)size);
	if (stream == NULL)
		return 0;

	IWICBitmapDecoder *decoder = NULL;
	HRESULT hr = uiprivWICFactory->CreateDecoderFromStream(
		stream,
		NULL,
		WICDecodeMetadataCacheOnDemand,
		&decoder);

	if (FAILED(hr)) {
		stream->Release();
		return 0;
	}

	IWICBitmapFrameDecode *frame = NULL;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) {
		decoder->Release();
		stream->Release();
		return 0;
	}

	IWICFormatConverter *converter = NULL;
	hr = uiprivWICFactory->CreateFormatConverter(&converter);
	if (FAILED(hr)) {
		frame->Release();
		decoder->Release();
		stream->Release();
		return 0;
	}

	hr = converter->Initialize(
		frame,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0,
		WICBitmapPaletteTypeMedianCut);

	if (FAILED(hr)) {
		converter->Release();
		frame->Release();
		decoder->Release();
		stream->Release();
		return 0;
	}

	hr = uiprivWICFactory->CreateBitmapFromSource(
		converter,
		WICBitmapCacheOnDemand,
		&iv->wicBitmap);

	converter->Release();
	frame->Release();
	decoder->Release();
	stream->Release();

	if (FAILED(hr))
		return 0;

	uiImageViewUpdateBitmap(iv);
	return 1;
}

int uiImageViewSetFromResource(uiImageView *iv, int resourceId)
{
	HINSTANCE hExe = GetModuleHandle(NULL);
	HRSRC hRes = FindResourceW(hExe, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
	if (hRes == NULL)
		return 0;

	DWORD size = SizeofResource(hExe, hRes);
	if (size == 0)
		return 0;

	HGLOBAL hGlobal = LoadResource(hExe, hRes);
	if (hGlobal == NULL)
		return 0;

	void *data = LockResource(hGlobal);
	if (data == NULL)
		return 0;

	return uiImageViewSetFromData(iv, data, size);
}

int uiImageViewWidth(uiImageView *iv)
{
	return iv->width;
}

int uiImageViewHeight(uiImageView *iv)
{
	return iv->height;
}

void uiImageViewSetSize(uiImageView *iv, int width, int height)
{
	iv->width = width;
	iv->height = height;
	uiImageViewUpdateBitmap(iv);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(iv));
}
