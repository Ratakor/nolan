/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <curl/curl.h>
#include <gd.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>

#include <err.h>
#include <string.h>

#include "nolan.h"

/* meh */
#define DIFF 110
#define WHITE 12000000

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static int write_rect(gdRect *rect, gdImagePtr im);

size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	if (size * nmemb > MAX_MESSAGE_LEN)
		errx(1, "%s:%d %s: fix your code", __FILE__, __LINE__, __func__);
	return strlcpy(stream, ptr, MAX_MESSAGE_LEN);
}

char *
curl(char *url)
{
	CURL *handle = curl_easy_init();
	char *buf = xmalloc(MAX_MESSAGE_LEN);

	*buf = '\0';
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, buf);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	return buf;
}

unsigned int
curl_file(char *url, char *fname)
{
	CURL *handle = curl_easy_init();
	CURLcode ret;
	FILE *fp = xfopen(fname, "wb");

	curl_easy_setopt(handle, CURLOPT_URL, url);
	/* curl uses fwrite by default */
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
	ret = curl_easy_perform(handle);
	fclose(fp);
	curl_easy_cleanup(handle);

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
		errx(1, "Failed tesseract recognition");

	txt_ocr = TessBaseAPIGetUTF8Text(handle);
	txt_out = xstrdup(txt_ocr);

	TessDeleteText(txt_ocr);
	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
	pixDestroy(&img);

	return txt_out;
}
