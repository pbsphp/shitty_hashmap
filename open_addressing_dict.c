#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "open_addressing_dict.h"


/**
 * Malloc. Exit on failure.
 */
static inline void *
safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        printf("fatal: Memory allocation failed\n");
        exit(1);
    }

    return ptr;
}


// Allocate at least `DICT_MIN_ARRAY_SIZE' cells for entries array.
// For DICT_MIN_ARRAY_SIZE = 8, 5 items can be added into dictionary without
// entries array resizing.
#define DICT_MIN_ARRAY_SIZE 8


#define ENTRY_EMPTY 0
#define ENTRY_OK 1
#define ENTRY_DELETED 2


/**
 * Is entry matches.
 */
static inline int
_is_entry_matches(
    struct dict_entry entry, unsigned int hash, const char *key)
{
    return (
        (entry.kind == ENTRY_OK || entry.kind == ENTRY_DELETED) &&
        entry.hash == hash &&
        strcmp(entry.key, key) == 0
    );
}


/**
 * Resize `entries_array' to `new_size' size.
 */
static void
_do_resize_array(struct dict *d, size_t new_size)
{
    struct dict_entry *new_array = safe_malloc(
        sizeof(struct dict_entry) * new_size);
    for (size_t i = 0; i < new_size; ++i) {
        new_array[i].kind = ENTRY_EMPTY;
    }

    for (size_t i = 0; i < d->array_allocated; ++i) {
        struct dict_entry *entry = &d->entries_array[i];
        if (entry->kind == ENTRY_OK) {
            unsigned int new_expected_position = entry->hash % new_size;

            struct dict_entry *new_entry = &new_array[new_expected_position];
            while (new_entry->kind != ENTRY_EMPTY) {
                new_expected_position = (new_expected_position + 1) % new_size;
                new_entry = &new_array[new_expected_position];
            }

            *new_entry = *entry;
        }
    }

    free(d->entries_array);
    d->entries_array = new_array;
    d->array_allocated = new_size;
}


/**
 * Resize `entries_array' if needed.
 */
static void
_resize_array_if_needed(struct dict *d)
{
    size_t min_size = d->len * 3 / 2;
    size_t max_size = d->len * 5;

    if (d->array_allocated < min_size || d->array_allocated > max_size) {
        size_t optimal_size = d->len * 2;
        if (optimal_size < DICT_MIN_ARRAY_SIZE) {
            optimal_size = DICT_MIN_ARRAY_SIZE;
        }
        if (d->array_allocated != optimal_size) {
            _do_resize_array(d, optimal_size);

        }
    }
}


/**
 * Create new dictionary object.
 */
struct dict *
dict_init(unsigned int (*hash_function)(const char *))
{
    struct dict *d = safe_malloc(sizeof(struct dict));
    d->len = 0;
    d->array_allocated = DICT_MIN_ARRAY_SIZE;
    d->entries_array = safe_malloc(
        sizeof(struct dict_entry) * d->array_allocated);
    for (size_t i = 0; i < d->array_allocated; ++i) {
        d->entries_array[i].kind = ENTRY_EMPTY;
    }

    d->hash_function = hash_function;

    return d;
}


/**
 * Destroy dictionary object.
 */
void
dict_destroy(struct dict *d)
{
    free(d->entries_array);
    free(d);
}


/**
 * Get value by key.
 */
const char *
dict_get(struct dict *d, const char *key)
{
    unsigned int hash = d->hash_function(key);
    unsigned int expected_position = hash % d->array_allocated;
    struct dict_entry *entry = &d->entries_array[expected_position];

    while (!_is_entry_matches(*entry, hash, key) &&
            entry->kind != ENTRY_EMPTY) {
        expected_position = (expected_position + 1) % d->array_allocated;
        entry = &d->entries_array[expected_position];
    }

    if (entry->kind == ENTRY_EMPTY || entry->kind == ENTRY_DELETED) {
        return NULL;
    }

    return entry->value;
}


/**
 * Set value by key.
 */
void
dict_set(struct dict *d, const char *key, const char *value)
{
    unsigned int hash = d->hash_function(key);
    unsigned int expected_position = hash % d->array_allocated;
    struct dict_entry *entry = &d->entries_array[expected_position];

    while (!_is_entry_matches(*entry, hash, key) &&
            entry->kind == ENTRY_OK) {
        expected_position = (expected_position + 1) % d->array_allocated;
        entry = &d->entries_array[expected_position];
    }

    if (entry->kind != ENTRY_OK) {
        ++d->len;
    }

    entry->kind = ENTRY_OK;
    entry->hash = hash;
    entry->key = key;
    entry->value = value;

    _resize_array_if_needed(d);
}


/**
 * Remove item by key.
 */
void
dict_del(struct dict *d, const char *key)
{
    unsigned int hash = d->hash_function(key);
    unsigned int expected_position = hash % d->array_allocated;

    struct dict_entry *entry = &d->entries_array[expected_position];
    while (!_is_entry_matches(*entry, hash, key) &&
            entry->kind != ENTRY_EMPTY) {
        expected_position = (expected_position + 1) % d->array_allocated;
        entry = &d->entries_array[expected_position];
    }

    if (entry->kind == ENTRY_OK) {
        entry->kind = ENTRY_DELETED;
        --d->len;

        _resize_array_if_needed(d);
    }
}


/**
 * Draw dict contents for debugging.
 */
void
dict_draw(struct dict *d)
{
    for (size_t i = 0; i < d->array_allocated; ++i) {
        struct dict_entry *entry = &d->entries_array[i];

        printf("%ld:\t", i);

        if (entry->kind != ENTRY_OK) {
            printf("-");
        } else {
            printf("%s:%s", entry->key, entry->value);
            if (entry->hash % d->array_allocated != i) {
                printf("    (must be %ld)", entry->hash % d->array_allocated);
            }
        }

        printf("\n");
    }
}
