#ifndef LINKED_LIST_DICT_H
#define LINKED_LIST_DICT_H


/**
 * One dict item.
 */
struct dict_entry
{
    unsigned int hash;
    const char *key;
    const char *value;
    // For collisions.
    struct dict_entry *neighbour;
};


/**
 * Dictionary object.
 */
struct dict
{
    // Array containing entries. Entry position in this array is determined by
    // hash function.
    struct dict_entry **entries_array;
    // Number of dictionary entries.
    size_t len;
    // `entries_array' full size including dictionary entries and empty cells.
    size_t array_allocated;
    // Number of dictionary entries in `entries_array'.
    size_t array_len;

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
