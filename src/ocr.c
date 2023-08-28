/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <curl/curl.h>
#include <gd.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>

#include <errno.h>
#include <string.h>

#include "nolan.h"

/* meh */
#define DIFF 110
/* #define WHITE 12000000 */
#define WHITE 10000000

struct Slice {
	char *data;
	size_t siz;
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static int write_rect(gdRect *rect, gdImagePtr im);

size_t
write_data(void *ptr, size_t siz, size_t nmemb, void *stream)
{
	struct Slice *buf;

	if (siz && nmemb > (size_t) -1 / siz)
		die(1, "realloc: %s", strerror(ENOMEM));

	siz *= nmemb;
	buf = (struct Slice *)stream;
	buf->data = xrealloc(buf->data, buf->siz + siz + 1);
	memcpy(buf->data + buf->siz, ptr, siz);
	buf->siz += siz;
	buf->data[buf->siz] = '\0';

	return buf->siz;
}

char *
curl(char *url)
{
	CURL *handle;
	struct Slice buf = { .data = NULL, .siz = 0 };

	handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buf);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	return buf.data;
}

CURLcode
curl_file(char *url, char *fname)
{
	CURL *handle;
	CURLcode code;
	FILE *fp;

	handle = curl_easy_init();
	fp = xfopen(fname, "wb");
	curl_easy_setopt(handle, CURLOPT_URL, url);
	/* curl uses fwrite by default */
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
	code = curl_easy_perform(handle);
	fclose(fp);
	curl_easy_cleanup(handle);

	return code;
}

int
write_rect(gdRect *rect, gdImage *im)
{
	int x = 0, y, X;

	y = (3 * gdImageSY(im)) / 4;
	X = gdImageSX(im);
	rect->y = 0;
	rect->height = gdImageSY(im);

	while (x < X && gdImageGetPixel(im, x++, y) < WHITE);
	if (x == X) {
		x = 0;
		y = (2 * gdImageSY(im)) / 3;
		while (x < X && gdImageGetPixel(im, x++, y) < WHITE);
		if (x == X)
			return 1;
	}

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

	fp = xfopen(fname, "rb");
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
	fp = xfopen(fname, "wb");
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
ocr(const char *fname, const char *lang)
{
	TessBaseAPI *handle;
	PIX *img;
	char *txt_ocr, *txt_out;

	if ((img = pixRead(fname)) == NULL) {
		log_error("Failed to read image (%s)", fname);
		return NULL;
	}

	handle = TessBaseAPICreate();
	if (TessBaseAPIInit3(handle, NULL, lang) != 0) {
		log_error("Failed to init tesseract (lang:%s)", lang);
		return NULL;
	}

	TessBaseAPISetImage2(handle, img);
	if (TessBaseAPIRecognize(handle, NULL) != 0)
		die(1, "Failed tesseract recognition");

	txt_ocr = TessBaseAPIGetUTF8Text(handle);
	txt_out = xstrdup(txt_ocr);

	TessDeleteText(txt_ocr);
	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
	pixDestroy(&img);

	return txt_out;
}
