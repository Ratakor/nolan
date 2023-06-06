#define LENGTH(X) (sizeof X / sizeof X[0])
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

void warn(const char *fmt, ...);
void die(const char *fmt, ...);
char *nstrchr(const char *s, int c, int n);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
int file_exists(char *filename);
void *emalloc(size_t size);
void *ecalloc(size_t nmemb, size_t size);
