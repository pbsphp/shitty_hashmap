#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linked_list_dict.h"


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


/**
 * Is entry matches.
 */
static inline int
_is_entry_matches(
    struct dict_entry entry, unsigned int hash, const char *key)
{
    return entry.hash == hash && strcmp(entry.key, key) == 0;
}


/**
 * Create array for dictionary entries with given size.
 */
static inline struct dict_entry **
_create_array(size_t size)
{
    struct dict_entry **new_array = safe_malloc(
        sizeof(struct dict_entry *) * size);
    for (size_t i = 0; i < size; ++i) {
        new_array[i] = NULL;
    }

    return new_array;
}


/**
 * Remove array with dictionary entries and free memory.
 */
static inline void
_delete_array(struct dict_entry **entries_array, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        struct dict_entry *entry = entries_array[i];
        while (entry != NULL) {
            struct dict_entry *next = entry->neighbour;
            free(entry);
            entry = next;
        }
    }

    free(entries_array);
}


/**
 * Move dictionary entries from `src_array' to `dst_array'.
 */
static void
_move_array(
    struct dict_entry **src_array,
    size_t src_size,
    struct dict_entry **dst_array,
    size_t dst_size)
{
    for (size_t i = 0; i < src_size; ++i) {
        struct dict_entry *entry = src_array[i];
        while (entry != NULL) {
            unsigned int new_position = entry->hash % dst_size;
            struct dict_entry *next = entry->neighbour;

            struct dict_entry *maybe_collided_entry = dst_array[new_position];
            // If `maybe_collided_entry' is not NULL, there is collision.
            // It's OK. Anyway, `entry' becomes linked list head.
            entry->neighbour = maybe_collided_entry;
            dst_array[new_position] = entry;

            entry = next;
        }
    }
}


/**
 * Resize `entries_array' to `new_size' size.
 */
static void
_do_resize_array(struct dict *d, size_t new_size)
{
    struct dict_entry **new_array = _create_array(new_size);
    _move_array(
        d->entries_array,
        d->array_allocated,
        new_array,
        new_size
    );
    free(d->entries_array);
    d->entries_array = new_array;
    d->array_allocated = new_size;

    size_t array_len = 0;
    for (int i = 0; i < new_size; ++i) {
        if (new_array[i] != NULL) {
            ++array_len;
        }
    }

    d->array_len = array_len;
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
        size_t optimal_size = d->len * 3;
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
    d->array_len = 0;
    d->array_allocated = DICT_MIN_ARRAY_SIZE;
    d->entries_array = _create_array(d->array_allocated);
    d->hash_function = hash_function;

    return d;
}


/**
 * Destroy dictionary object.
 */
void
dict_destroy(struct dict *d)
{
    _delete_array(d->entries_array, d->array_allocated);
    free(d);
}


/**
 * Get value by key.
 */
const char *
dict_get(struct dict *d, const char *key)
{
    unsigned int hash = d->hash_function(key);
    unsigned int position = hash % d->array_allocated;
    struct dict_entry *entry = d->entries_array[position];

    while (entry != NULL) {
        if (_is_entry_matches(*entry, hash, key)) {
            return entry->value;
        }

        entry = entry->neighbour;
    }

    return NULL;
}


/**
 * Set value by key.
 */
void
dict_set(struct dict *d, const char *key, const char *value)
{
    unsigned int hash = d->hash_function(key);
    unsigned int position = hash % d->array_allocated;
    struct dict_entry *entry = d->entries_array[position];

    while (entry != NULL) {
        if (_is_entry_matches(*entry, hash, key)) {
            entry->value = value;

            return;
        }

        entry = entry->neighbour;
    }

    struct dict_entry *new_entry = safe_malloc(sizeof(struct dict_entry));
    new_entry->hash = hash;
    new_entry->key = key;
    new_entry->value = value;

    struct dict_entry *existing_entry = d->entries_array[position];
    // There is collision if `existing_entry' is not NULL. Anyway, new entry
    // becomes linked list head.
    new_entry->neighbour = existing_entry;
    d->entries_array[position] = new_entry;

    ++d->len;
    if (existing_entry == NULL) {
        ++d->array_len;
    }

    _resize_array_if_needed(d);
}


/**
 * Remove item by key.
 */
void
dict_del(struct dict *d, const char *key)
{
    unsigned int hash = d->hash_function(key);
    unsigned int position = hash % d->array_allocated;
    struct dict_entry *entry = d->entries_array[position];
    // After entry deletion we shoud restore liked list. `prev_entry' is
    // left entry's neighbour (or NULL if entry is head of linked list).
    struct dict_entry *prev_entry = NULL;

    while (entry != NULL) {
        if (_is_entry_matches(*entry, hash, key)) {
            if (prev_entry == NULL) {
                d->entries_array[position] = entry->neighbour;
                --d->array_len;
            } else {
                prev_entry->neighbour = entry->neighbour;
            }

            free(entry);
            --d->len;
        }

        prev_entry = entry;
        entry = entry->neighbour;
    }

    _resize_array_if_needed(d);
}


/**
 * Draw dict contents for debugging.
 */
void
dict_draw(struct dict *d)
{
    for (size_t i = 0; i < d->array_allocated; ++i) {
        struct dict_entry *entry = d->entries_array[i];

        printf("%ld:\t", i);

        if (entry == NULL) {
            printf("-");
        } else {
            while (entry != NULL) {
                printf("%s:%s    ", entry->key, entry->value);
                entry = entry->neighbour;
            }
        }

        printf("\n");
    }
}
