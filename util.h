#define LENGTH(X) (int)(sizeof X / sizeof X[0])

void die(const char *errstr, ...);
char *nstrchr(const char *s, int c, int n);
