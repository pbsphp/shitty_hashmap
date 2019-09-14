#ifndef COMPACT_DICT_H
#define COMPACT_DICT_H

#include <stdbool.h>


/**
 * One dict item.
 */
struct dict_entry
{
    unsigned int hash;
    const char *key;
    const char *value;
    bool is_alive;
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

    // Hash function
    unsigned int (*hash_function)(const char *);
};


/**
 * Create new dictionary object.
 */
struct dict *
dict_init(unsigned int (*hash_function)(const char *));


/**
 * Destroy dictionary object.
 */
void
dict_destroy(struct dict *);


/**
 * Get value by key.
 */
const char *
dict_get(struct dict *, const char *);


/**
 * Set value by key.
 */
void
dict_set(struct dict *, const char *, const char *);


/**
 * Remove item by key.
 */
void
dict_del(struct dict *, const char *);


/**
 * Draw dict contents for debugging.
 */
void
dict_draw(struct dict *);


#endif
