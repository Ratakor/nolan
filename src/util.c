#include <sys/stat.h>

#include "nolan.h"

int
file_exists(const char *filename)
{
	struct stat buf;

	return (stat(filename, &buf) == 0);
}

FILE *
xfopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		die(1, "fopen: '%s' [%s]", filename, mode);

	return fp;
}
