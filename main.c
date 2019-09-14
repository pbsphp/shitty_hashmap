#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "linked_list_dict.h"
// #include "open_addressing_dict.h"
#include "compact_dict.h"


/**
 * Simple hash function for strings.
 */
static inline unsigned int
hash_function(const char *str)
{
    unsigned int result = 42;
    char p;
    unsigned int n = 0;
    while ((p = *str++)) {
        result += ((unsigned int) p) << (n++ % 48);
        result += (unsigned int) p;
    }

    return result;
}


int main()
{
    const int iters = 10;

    char keys_buffer[iters][10];
    char vals_buffer[iters][10];

    for (int i = 0; i < iters; ++i) {
        sprintf(keys_buffer[i], "key%d", i);
        sprintf(vals_buffer[i], "value%d", i);
    }

    struct dict *d = dict_init(&hash_function);

    for (int i = 0; i < iters; ++i) {
        dict_set(d, keys_buffer[i], vals_buffer[i]);
    }

    for (int i = 0; i < iters; ++i) {
        const char *key = keys_buffer[i];
        const char *val = vals_buffer[i];
        const char *real_val = dict_get(d, key);
        if (strcmp(val, real_val) != 0) {
            printf(
                "%s: %s expected, %s got.\n",
                key,
                val,
                real_val);
        }
    }

    for (int i = 0; i < iters; ++i) {
        dict_del(d, keys_buffer[i]);
    }

    dict_set(d, "asdasd", "QWEQWE");
    dict_set(d, "dsadsa", "EWQEWQ");

    dict_del(d, "nonexisting");
    dict_del(d, "asdasd");

    dict_draw(d);

    printf("\nLen: %ld\n", d->len);
    printf("asdasd=%s, dsadsa=%s\n", dict_get(d, "asdasd"), dict_get(d, "dsadsa"));

    dict_destroy(d);

    return 0;
}
