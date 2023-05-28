#define LENGTH(X) (sizeof X / sizeof X[0])
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

void die(const char *errstr, ...);
char *nstrchr(const char *s, int c, int n);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
size_t cpstr(char *dst, const char *src, size_t siz);
size_t catstr(char *dst, const char *src, size_t siz);
