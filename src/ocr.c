#include <string.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include <curl/curl.h>
#include "nolan.h"

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);

static size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

void
curl(char *url, char *fname)
{
	CURL *handle;
	FILE *fp;

	curl_global_init(CURL_GLOBAL_ALL);

	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);

	if ((fp = fopen(fname, "wb")) == NULL)
		die("nolan: Failed to open %s\n", fname);

	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
	curl_easy_perform(handle);

	fclose(fp);
	curl_easy_cleanup(handle);
	curl_global_cleanup();
}

char *
ocr(char *fname)
{
	TessBaseAPI *handle;
	PIX *img;
	char *txt_ocr, *txt_out;

	if ((img = pixRead(fname)) == NULL)
		die("nolan: Error reading image\n");

	handle = TessBaseAPICreate();
	if (TessBaseAPIInit3(handle, NULL, "eng") != 0)
		die("nolan: Error initialising tesseract\n");

	TessBaseAPISetImage2(handle, img);
	if (TessBaseAPIRecognize(handle, NULL) != 0)
		die("nolan: Error in tesseract recognition\n");

	txt_ocr = TessBaseAPIGetUTF8Text(handle);
	txt_out = strdup(txt_ocr);

	TessDeleteText(txt_ocr);
	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
	pixDestroy(&img);

	return txt_out;
}

