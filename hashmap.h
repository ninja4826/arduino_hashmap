/**
 * hashmap.h - Library that provides simple hashmap implementation
 *     for Arduino.
 * Created by Patrick Trentin, March 22, 2014.
 * Released into the public domain.
 *
 * WARNING(s):
 * - you don't want to use Hashmaps on Arduino, unless you really do
 * - it is NOT designed for concurrency (NOT thread-safe)
 * - uses dynamic memory allocation: initial allocation should be large
 *     enough to avoid re-hashing later on (expensive operation)
 * - does not retain (nor copy) memory associated to key/value pairs,
 *     anything you free/remove will cause dangling pointers within
 *     the hashmap, unless you first remove the key/value pair from
 *     the hashmap itself in advance
 *
 * NOTE(s):
 * - C-style implementation
 * - uses Bob Jenkins' one-at-a-time hash function [ref: Wikipedia]
 * - if your hashmap is really scattered and does not change its content
 *     too often, consider using ``hashmap_keys()'' instead of
 *     ``hashmap_iterate'' to apply a function to all stored key/value pairs
 * - if you find any bug within this library, consider contacting the author:
 *     patrick [.] trentin [.] 88 [@] gmail [.] com
 */

#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <Arduino.h>

/******************************************************************************/
/*** CONSTANTS AND DEFINITIONS                                              ***/
/******************************************************************************/

#define HASHMAP_DEFAULT_SIZE 16

#define HASHMAP_OK 0
#define HASHMAP_ERR -1
#define HASHMAP_FULL -2
#define HASHMAP_EMPTY -3
#define HASHMAP_NOT_FOUND -4

typedef void any_key_t;
typedef void any_value_t;

typedef void hashmap_t;
typedef hashmap_t* hashmap_ptr;

typedef int (*hashmap_key_compare) (const any_key_t*, const any_key_t*);
typedef uint32_t (*hashmap_key_hash) (const any_key_t*);
typedef int (*hashmap_fun) (void*, any_value_t*);

/******************************************************************************/
/*** PUBLIC INTERFACE                                                       ***/
/******************************************************************************/

/**
 * hashmap_new:
 * returns a new hashmap of size ``max_size'' which keys can be compared/hashed
 * using ``compare_fun''/``hash_fun'' functions.
 * NOTE: if such functions are undefined, the hashmap defaults to the ``char*''
 *     implementation for strings (assumes ``any_key_t'' is ``char'').
 */
hashmap_ptr hashmap_new(const size_t max_size,
    hashmap_key_compare compare_fun, hashmap_key_hash hash_fun);

/**
 * hashmap_delete:
 * destroys a hashmap and its associated structures
 * NOTE: the hashmap does not retain memory associated to key/value pairs
 *     therefore it will not finalize them as well.
 */
void hashmap_delete(const hashmap_ptr map);

/**
 * hashmap_put:
 * puts key/value pair into hashmap if any bucket available
 * NOTE: overwrites any bucket with same key
 */
int hashmap_put(hashmap_ptr map, const any_key_t* key, const any_value_t* value);

/**
 * hashmap_get:
 * returns the value associated to a key within the hashmap, if any
 */
int hashmap_get(hashmap_ptr map, const any_key_t* key, any_value_t** value);

/**
 * hashmap_remove:
 * removes key/value pair from hashmap, if found;
 * stores the value in the output variable on return
 */
int hashmap_remove(hashmap_ptr map, const any_key_t* key, any_value_t** value);

/**
 * hashmap_keys:
 * returns an array containing all keys in use within the hashmap
 */
int hashmap_keys(hashmap_ptr map, any_key_t*** keys, size_t* size);

/**
 * hashmap_iterate:
 * iteratively applies function ``fun'' to each value within the hashmap
 * using ``data'' as additional parameter. Does not check ``fun'' exit code
 */
int hashmap_iterate(hashmap_ptr map, hashmap_fun fun, void* data);

/**
 * hashmap_iterate_check_ok_status:
 * iteratively applies function ``fun'' to each value within the hashmap
 * using ``data'' as additional parameter. Early stops if ``fun'' exit code
 * is different than HASHMAP_OK (which evaluates to 0).
 */
int hashmap_iterate_check_ok_status(hashmap_ptr map, hashmap_fun fun, void* data);

#endif
