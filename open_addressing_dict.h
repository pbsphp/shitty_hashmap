#ifndef OPEN_ADDRESSING_DICT_H
#define OPEN_ADDRESSING_DICT_H


/**
 * One dict item.
 */
struct dict_entry
{
    unsigned int hash;
    const char *key;
    const char *value;
    // Kind of item. It can be normal, empty or deleted.
    int kind;
};


/**
 * Dictionary object.
 */
struct dict
{
    // Array containing entries. Entry position in this array is determined by
    // hash function.
    struct dict_entry *entries_array;
    // Number of dictionary entries.
    size_t len;
    // `entries_array' length.
    size_t array_allocated;

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
