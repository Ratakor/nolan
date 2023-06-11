#include <curl/curl.h>
#include <gd.h>
#include <leptonica/allheaders.h>
#include <string.h>
#include <tesseract/capi.h>

#include "nolan.h"

/* meh */
#define DIFF 110
#define WHITE 12000000

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static int write_rect(gdRect *rect, gdImagePtr im);

static size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

unsigned int
curl(char *url, char *fname)
{
	CURL *handle;
	CURLcode ret;
	FILE *fp;

	curl_global_init(CURL_GLOBAL_ALL);

	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);

	fp = efopen(fname, "wb");
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
	ret = curl_easy_perform(handle);

	fclose(fp);
	curl_easy_cleanup(handle);
	curl_global_cleanup();

	return ret;
}

int
write_rect(gdRect *rect, gdImage *im)
{
	rect->y = 0;
	rect->height = gdImageSY(im);
	int x = 0, y, X = gdImageSX(im);
	y = (3 * gdImageSY(im)) / 4;

	while (x < X && gdImageGetPixel(im, x++, y) < WHITE);
	if (x == X)
		return 1;
	if (x > 40)
		x += DIFF + 5;
	else
		x += DIFF;
	rect->x = x;
	rect->width = X - x;
	return 0;
}

/* type: 0 = jpeg, 1 (or anything else) = png */
int
crop(char *fname, int type)
{
	FILE *fp;
	gdImage *im, *cropped;
	gdRect rect;

	fp = efopen(fname, "rb");
	if (type == 0)
		im = gdImageCreateFromJpeg(fp);
	else
		im = gdImageCreateFromPng(fp);
	fclose(fp);

	if (im == NULL)
		return 1;
	if (write_rect(&rect, im) == 1)
		return 1;

	cropped = gdImageCrop(im, &rect);
	fp = efopen(fname, "wb");
	if (type == 0)
		gdImageJpeg(cropped, fp, 100);
	else
		gdImagePng(cropped, fp);
	fclose(fp);
	gdImageDestroy(cropped);
	gdImageDestroy(im);

	return 0;
}

char *
ocr(char *fname, char *lang)
{
	TessBaseAPI *handle;
	PIX *img;
	char *txt_ocr, *txt_out;

	if ((img = pixRead(fname)) == NULL) {
		WARN("failed to read image (%s)", fname);
		return NULL;
	}

	handle = TessBaseAPICreate();
	if (TessBaseAPIInit3(handle, NULL, lang) != 0)
		DIE("failed to init tesseract (lang:%s)", lang);

	TessBaseAPISetImage2(handle, img);
	if (TessBaseAPIRecognize(handle, NULL) != 0)
		DIE("failed tesseract recognition");

	txt_ocr = TessBaseAPIGetUTF8Text(handle);
	txt_out = strdup(txt_ocr);

	TessDeleteText(txt_ocr);
	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
	pixDestroy(&img);

	return txt_out;
}
