#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/**
 * Malloc. Exit on failure.
 */
void *safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        printf("fatal: Memory allocation failed\n");
        exit(1);
    }

    return ptr;
}


/**
 * Realloc. Exit on failure.
 */
void *safe_realloc(void *mem, size_t size)
{
    void *ptr = realloc(mem, size);
    if (ptr == NULL) {
        printf("fatal: Memory allocation failed\n");
        exit(1);
    }

    return ptr;
}


/**
 * Simple hash function for strings.
 */
unsigned int hash_function(const char *str)
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


// Allocate at least `DICT_MIN_ARRAY_SIZE' cells for entries array and index
// array.
#define DICT_MIN_ARRAY_SIZE 8


// Index value for empty items.
#define ENTRY_EMPTY -1


/**
 * One dict item.
 */
struct dict_entry
{
    unsigned int hash;
    const char *key;
    const char *value;
    int is_alive;
};


/**
 * Dictionary object.
 */
struct dict
{
    struct dict_entry *entries_array;
    void *index_array;
    size_t index_array_size;
    size_t index_array_item_size;

    // Number of dictionary entries.
    size_t len;

    // `entries_array' size (without free slots).
    size_t entries_array_size;
    // `entries_array' full size (icluding free slots).
    size_t entries_array_allocated;
};


/**
 * Return value of Nth item from given index array.
 */
int _index_array_get(void *arr, size_t item_size, int idx)
{
    int res;
    switch (item_size) {
    case sizeof(int8_t):
        res = ((int8_t *) arr)[idx];
        break;
    case sizeof(int16_t):
        res = ((int16_t *) arr)[idx];
        break;
    case sizeof(int32_t):
        res = ((int32_t *) arr)[idx];
        break;
    default:
        res = ((int64_t *) arr)[idx];
    }

    return res;
}


/**
 * Set value of Nth item of given index array.
 */
void _index_array_set(void *arr, size_t item_size, int idx, int value)
{
    switch (item_size) {
    case sizeof(int8_t):
        ((int8_t *) arr)[idx] = value;
        break;
    case sizeof(int16_t):
        ((int16_t *) arr)[idx] = value;
        break;
    case sizeof(int32_t):
        ((int32_t *) arr)[idx] = value;
        break;
    default:
        ((int64_t *) arr)[idx] = value;
    }
}


/**
 * Allocate new index array.
 */
void *_index_array_init(size_t size, size_t item_size)
{
    void *arr = safe_malloc(size * item_size);

    // TODO: Rewrite using _Generic or #define.
    for (size_t i = 0; i < size; ++i) {
        _index_array_set(arr, item_size, i, ENTRY_EMPTY);
    }

    return arr;
}


/**
 * Destroy index array.
 */
void _index_array_destroy(void *arr)
{
    free(arr);
}


/**
 * Resize array items.
 *
 * Creates new array with same length, but other item size. For example:
 * replace index array of int8_t to int16_t and vice versa.
 */
void *_index_array_resize_items(
    void *arr, size_t arr_size, size_t old_item_size, size_t new_item_size)
{
    void *new_arr = _index_array_init(arr_size, new_item_size);
    for (size_t i = 0; i < arr_size; ++i) {
        int val = _index_array_get(arr, old_item_size, i);
        _index_array_set(new_arr, new_item_size, i, val);
    }

    _index_array_destroy(arr);

    return new_arr;
}


/**
 * Allocate entries array.
 */
struct dict_entry *_entries_array_init(size_t size)
{
    return safe_malloc(sizeof(struct dict_entry) * size);
}


/**
 * Destroy entries array.
 */
void _entries_array_destroy(struct dict_entry *arr)
{
    free(arr);
}


/**
 * Is entry matches.
 */
int _is_entry_matches(
    struct dict_entry entry, unsigned int hash, const char *key)
{
    return (entry.hash == hash && strcmp(entry.key, key) == 0);
}


/**
 * Return size of index array item by entries array size.
 */
size_t _get_index_array_item_size_by_entries_array_size(size_t size)
{
    size_t index_array_item_size;
    if (size <= INT8_MAX) {
        index_array_item_size = sizeof(int8_t);
    } else if (size <= INT16_MAX) {
        index_array_item_size = sizeof(int16_t);
    } else if (size <= INT32_MAX) {
        index_array_item_size = sizeof(int32_t);
    } else {
        index_array_item_size = sizeof(int64_t);
    }

    return index_array_item_size;
}


/**
 * Rebuild index array. May change array size and item sizes.
 */
void _rebuild_index_array(struct dict *d)
{
    size_t index_array_item_size = \
        _get_index_array_item_size_by_entries_array_size(
            d->entries_array_size);

    _index_array_destroy(d->index_array);
    d->index_array_item_size = index_array_item_size;
    d->index_array_size = d->entries_array_size * 2;
    d->index_array = _index_array_init(
        d->index_array_size, d->index_array_item_size);

    for (size_t i = 0; i < d->entries_array_size; ++i) {
        struct dict_entry *entry = &d->entries_array[i];
        if (entry->is_alive) {
            size_t index_pos = entry->hash % d->index_array_size;
            int index_val = _index_array_get(
                d->index_array, d->index_array_item_size, index_pos);
            while (index_val != ENTRY_EMPTY) {
                index_pos = (index_pos + 1) % d->index_array_size;
                index_val = _index_array_get(
                    d->index_array, d->index_array_item_size, index_pos);
            }

            _index_array_set(
                d->index_array, d->index_array_item_size, index_pos, i);
        }
    }
}


/**
 * Recreate entries array. Useful when there are a lot of deleted items.
 */
void _recreate_entries_array(struct dict *d)
{
    // TODO: Optimize: compress without recreation and copying.

    struct dict_entry *arr = d->entries_array;

    size_t new_size = d->len * 2;
    if (new_size < DICT_MIN_ARRAY_SIZE) {
        new_size = DICT_MIN_ARRAY_SIZE;
    }
    struct dict_entry *new_arr = _entries_array_init(new_size);
    struct dict_entry *new_arr_p = new_arr;

    for (size_t i = 0; i < d->entries_array_size; ++i) {
        if (arr[i].is_alive) {
            *new_arr_p++ = arr[i];
        }
    }

    _entries_array_destroy(arr);
    d->entries_array = new_arr;
    d->entries_array_size = d->len;
    d->entries_array_allocated = new_size;

    _rebuild_index_array(d);
}


/**
 * Check is time to rebuild index array.
 */
int _is_time_to_rebuild_index(struct dict *d)
{
    size_t index_array_item_size = \
        _get_index_array_item_size_by_entries_array_size(
            d->entries_array_size);

    size_t min_size = d->entries_array_size * 3 / 2;
    size_t max_size = d->entries_array_size * 3;

    return (
        index_array_item_size != d->index_array_item_size ||
        d->index_array_size < min_size ||
        d->index_array_size > max_size
    ) && d->entries_array_size * 2 > DICT_MIN_ARRAY_SIZE;
}


/**
 * Check is time to shrink entries array.
 */
int _is_time_to_shrink_entries_array(struct dict *d)
{
    return (
        d->entries_array_size > (d->len * 3) ||
        d->entries_array_allocated > (d->entries_array_size * 3)
    ) && d->len > DICT_MIN_ARRAY_SIZE;
}


/**
 * Grow `entries_array'.
 */
void _grow_entries_array(struct dict *d)
{
    size_t new_size;
    if (d->entries_array_size > 4096) {
        new_size = d->entries_array_size + 1024;
    } else {
        new_size = d->entries_array_size * 2;
    }

    d->entries_array = safe_realloc(
        d->entries_array, sizeof(struct dict_entry) * new_size);
    d->entries_array_allocated = new_size;

    if (_is_time_to_rebuild_index(d)) {
        _rebuild_index_array(d);
    }
}


/**
 * Return pointer to entry (or NULL) by position in index.
 */
struct dict_entry *_get_entry_by_index_pos(struct dict *d, int index_pos)
{
    struct dict_entry *entry = NULL;
    int index_val = _index_array_get(
        d->index_array, d->index_array_item_size, index_pos);
    if (index_val == ENTRY_EMPTY) {
        entry = NULL;
    } else {
        entry = &d->entries_array[index_val];
    }

    return entry;
}


/**
 * Create new dictionary object.
 */
struct dict *dict_init()
{
    struct dict *d = safe_malloc(sizeof(struct dict));
    d->len = 0;

    d->entries_array = _entries_array_init(DICT_MIN_ARRAY_SIZE);
    d->entries_array_allocated = DICT_MIN_ARRAY_SIZE;
    d->entries_array_size = 0;

    d->index_array = _index_array_init(DICT_MIN_ARRAY_SIZE, sizeof(int8_t));
    d->index_array_size = DICT_MIN_ARRAY_SIZE;
    d->index_array_item_size = sizeof(int8_t);

    return d;
}


/**
 * Destroy dictionary object.
 */
void dict_destroy(struct dict *d)
{
    _entries_array_destroy(d->entries_array);
    _index_array_destroy(d->index_array);
    free(d);
}


/**
 * Get value by key.
 */
const char *dict_get(struct dict *d, const char *key)
{
    unsigned int hash = hash_function(key);
    unsigned int index_pos = hash % d->index_array_size;

    struct dict_entry *entry = _get_entry_by_index_pos(d, index_pos);
    while (entry != NULL && !_is_entry_matches(*entry, hash, key)) {
        index_pos = (index_pos + 1) % d->index_array_size;
        entry = _get_entry_by_index_pos(d, index_pos);
    }

    return (entry != NULL && entry->is_alive) ? entry->value : NULL;
}


/**
 * Set value by key.
 */
void dict_set(struct dict *d, const char *key, const char *value)
{
    unsigned int hash = hash_function(key);
    unsigned int index_pos = hash % d->index_array_size;
    struct dict_entry *entry = _get_entry_by_index_pos(d, index_pos);

    while (entry != NULL && !_is_entry_matches(*entry, hash, key)) {
        index_pos = (index_pos + 1) % d->index_array_size;
        entry = _get_entry_by_index_pos(d, index_pos);
    }

    if (entry == NULL) {
        // Entry is not found, so we should add new entry into entries array.
        // Make sure there is empty slot.
        if (d->entries_array_size == d->entries_array_allocated) {
            // No empty places. We must increase entries array. Index may be
            // rebuilt, so we should recalculate index position.
            _grow_entries_array(d);

            index_pos = hash % d->index_array_size;
            int index_val = _index_array_get(
                d->index_array, d->index_array_item_size, index_pos);
            while (index_val != ENTRY_EMPTY) {
                index_pos = (index_pos + 1) % d->index_array_size;
                index_val = _index_array_get(
                    d->index_array, d->index_array_item_size, index_pos);
            }
        }

        int new_entry_pos = d->entries_array_size++;
        entry = &d->entries_array[new_entry_pos];
        entry->is_alive = 1;
        ++d->len;

        _index_array_set(
            d->index_array,
            d->index_array_item_size,
            index_pos,
            new_entry_pos);
    } else if (!entry->is_alive) {
        ++d->len;
        entry->is_alive = 1;
    }

    entry->hash = hash;
    entry->key = key;
    entry->value = value;

    if (_is_time_to_rebuild_index(d)) {
        _rebuild_index_array(d);
    }
}


/**
 * Remove item by key.
 */
void dict_del(struct dict *d, const char *key)
{
    unsigned int hash = hash_function(key);
    unsigned int index_pos = hash % d->index_array_size;

    struct dict_entry *entry = _get_entry_by_index_pos(d, index_pos);
    while (entry != NULL && !_is_entry_matches(*entry, hash, key)) {
        index_pos = (index_pos + 1) % d->index_array_size;
        entry = _get_entry_by_index_pos(d, index_pos);
    }

    if (entry != NULL && entry->is_alive) {
        --d->len;
        entry->is_alive = 0;

        if (_is_time_to_shrink_entries_array(d)) {
            _recreate_entries_array(d);
        } else if (_is_time_to_rebuild_index(d)) {
            _rebuild_index_array(d);
        }
    }
}


void _draw(struct dict *d)
{
    printf("Index ");
    switch(d->index_array_item_size) {
    case sizeof(int8_t):
        printf("(int8_t)");
        break;
    case sizeof(int16_t):
        printf("(int16_t)");
        break;
    case sizeof(int32_t):
        printf("(int32_t)");
        break;
    default:
        printf("(int64_t)");
    }
    printf("\n");

    for (size_t i = 0; i < d->index_array_size; ++i) {
        int val = _index_array_get(
            d->index_array, d->index_array_item_size, i);

        printf("%ld:\t", i);

        if (val != ENTRY_EMPTY) {
            printf("-> %d\n", val);
        } else {
            printf("-\n");
        }
    }

    printf("\nValues:\n");

    for (size_t i = 0; i < d->entries_array_size; ++i) {
        printf("%ld:\t", i);

        struct dict_entry *entry = &d->entries_array[i];

        if (entry->is_alive) {
            printf("%s:%s\n", entry->key, entry->value);
        } else {
            printf("-\n");
        }
    }
}


int main()
{
    struct dict *d = dict_init();

    for (int i = 0; i < 10000; ++i) {
        char *key = malloc(sizeof(char) * 10);
        char *val = malloc(sizeof(char) * 10);
        sprintf(key, "key%d", i);
        sprintf(val, "val%d", i);

        dict_set(d, key, val);
    }

    for (int i = 0; i < 10000; ++i) {
        char *key = malloc(sizeof(char) * 10);
        char *val = malloc(sizeof(char) * 10);
        sprintf(key, "key%d", i);
        sprintf(val, "val%d", i);

        const char *realval = dict_get(d, key);
        if (strcmp(val, realval) != 0) {
            printf("%s: %s expected, %s got.\n", key, val, realval);
        }
    }

    for (int i = 0; i < 10000; ++i) {
        char *key = malloc(sizeof(char) * 10);
        sprintf(key, "key%d", i);

        dict_del(d, key);
    }

    dict_set(d, "asdasd", "QWEQWE");
    dict_set(d, "dsadsa", "EWQEWQ");

    dict_del(d, "nonexisting");
    dict_del(d, "asdasd");

    _draw(d);

    printf("\nLen: %ld\n", d->len);
    printf("asdasd=%s, dsadsa=%s\n", dict_get(d, "asdasd"), dict_get(d, "dsadsa"));

    dict_destroy(d);

    return 0;
}
