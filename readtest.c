#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

int main() {
    FILE *f = fopen("test.txt", "r");
    errno = 0;
    assert(f);

    char buf[5] = {0};
    int bytes = fread(buf, 1, 4, f);
    printf("%d: '%s', eof: %d, error: %d: %d\n", bytes, buf, feof(f), ferror(f), errno);

    memset(buf, 0, 5);
    errno = 0;
    bytes = fread(buf, 1, 4, f);
    printf("%d: '%s', eof: %d, error: %d: %d\n", bytes, buf, feof(f), ferror(f), errno);

    memset(buf, 0, 5);
    errno = 0;
    bytes = fread(buf, 1, 4, f);
    printf("%d: '%s', eof: %d, error: %d: %d\n", bytes, buf, feof(f), ferror(f), errno);

    fclose(f);
}
