/* Minimal freestanding ABI helpers for Genesis objects compiled with clang -Oz.
 * No libc linkage. */
typedef unsigned int size_t;

void *__aeabi_memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *__aeabi_memcpy4(void *dst, const void *src, size_t n)
{
    return __aeabi_memcpy(dst, src, n);
}

void *__aeabi_memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}

void *__aeabi_memset(void *dst, size_t n, int c)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--)
        *d++ = (unsigned char)c;
    return dst;
}
