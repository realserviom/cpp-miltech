#include <ctype.h>
#include <string.h>

// Допоміжна функція для перетворення шістнадцяткового символу в число
static int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Повне URL-декодування рядка
void url_decode(char *dst, const char *src) {
    char *p = dst;
    while (*src) {
        if (*src == '+') {
            *p++ = ' '; // Плюс стає пробілом (для form-urlencoded)
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            int high = hex_to_int(src[1]);
            int low = hex_to_int(src[2]);
            if (high >= 0 && low >= 0) {
                *p++ = (char)((high << 4) | low);
                src += 3;
            } else {
                *p++ = *src++;
            }
        } else {
            *p++ = *src++;
        }
    }
    *p = '\0';
}