/**
 * hashmap.h - Library that provides simple hashmap implementation
 *     for Arduino.
 * Created by Patrick Trentin, March 22, 2014.
 * Released into the public domain.
 *
 * NOTE(s):
 * - if you find any bug within this library, consider contacting the author:
 *     patrick [.] trentin [.] 88 [@] gmail [.] com
 */
#include "hashmap.h"

/******************************************************************************/
/*** INTERNAL DATA STRUCTURES                                               ***/
/******************************************************************************/

typedef struct _hashmap_bucket {
    any_key_t* _key;
    any_value_t* _value;
    byte _in_use;
} hashmap_bucket;

typedef struct _hashmap_map {
    /* Storage */
    hashmap_bucket* _buckets;
    any_key_t** _keys;
    /* Hashing Functions */
    hashmap_key_compare _compare;
    hashmap_key_hash _hash;
    /* Hashmap Info */
    size_t _max_size;
    size_t _cur_size;
    byte _changed;
} hashmap_map;

/******************************************************************************/
/*** PRIVATE INTERFACE                                                      ***/
/******************************************************************************/

/**
 * hashmap_compare_str:
 * compares two '\0'-terminated strings, returning 0 if they are equal
 * NOTE: by default, any_key_t is assumed to be ``char'' and this function
 *     is used to compare two keys for equality.
 */
static inline int hashmap_key_compare_str(const char* key1, const char* key2);

/**
 * hashmap_hash_str:
 * computes the hash of a '\0'-terminated string using
 * Bob Jenkins one-at-a-time hash algorithm. [ref: Wikipedia]
 * NOTE: on Arduino we redefined uint32_t as ``unsigned long'';
 * NOTE: by default, any_key_t is assumed to be ``char'' and this function
 *     is used to compare two keys for equality.
 */
static inline uint32_t hashmap_key_hash_str(const char* key);

/**
 * hashmap_hash:
 * returns the index of a bucket in which key/value couple can be
 * stored, if any. Error code otherwise.
 */
static inline int hashmap_hash(hashmap_map* map, const any_key_t* key);

/**
 * hashmap_rehash:
 * doubles the size of the hashmap, and rehashes all the elements
 * NOTE: this function is particularly expensive, consider initializing
 *     your hashmap to be large enough right from the start
 */
static inline int hashmap_rehash(hashmap_map* map);

/**
 * hashmap_find:
 * returns the index of a bucket in which key/value couple is
 * stored, if any. Error code otherwise.
 */
static inline int hashmap_find(hashmap_map* map, const any_key_t* key);

/**
 * hashmap_iterate_impl:
 * iteratively applies function ``fun'' to each value within the hashmap
 * using ``data'' as additional parameter. When ``check_status'' != 0, early stops
 * if ``fun'' exit code is different than HASHMAP_OK (0).
 */
static inline int hashmap_iterate_impl(hashmap_ptr map,
    hashmap_fun fun, void* data, byte check_status);

/******************************************************************************/
/*** PUBLIC INTERFACE IMPLEMENTATION                                        ***/
/******************************************************************************/

/**
 * hashmap_new:
 * returns a new hashmap of size ``max_size'' which keys can be compared/hashed
 * using ``compare_fun''/``hash_fun'' functions.
 * NOTE: if such functions are undefined, the hashmap defaults to the ``char*''
 *     implementation for strings (assumes ``any_key_t'' is ``char'').
 */
hashmap_ptr hashmap_new(const size_t max_size,
    hashmap_key_compare compare_fun, hashmap_key_hash hash_fun) {

    /* alloc hashmap */
    hashmap_map* map = (hashmap_map*) calloc(1, sizeof(hashmap_map));
    if (!map)
        goto error;

    /* check input parameters */
    if (max_size <= 0) {
        map->_max_size = HASHMAP_DEFAULT_SIZE;
    } else {
        map->_max_size = max_size;
    }

    /* alloc support memory */
    map->_buckets = (hashmap_bucket*)
                             calloc(map->_max_size, sizeof(hashmap_bucket));
    if (!map->_buckets)
        goto error;

    /* initialize fields */
    map->_keys = NULL;
    map->_compare = (compare_fun ? compare_fun :
                    (hashmap_key_compare) hashmap_key_compare_str);
    map->_hash = (hash_fun ? hash_fun :
                    (hashmap_key_hash) hashmap_key_hash_str);
    map->_cur_size = 0;
    map->_changed = 1;

    return (hashmap_ptr) map;

error:
    hashmap_delete(map);
    return NULL;
}

/**
 * hashmap_delete:
 * destroys a hashmap and its associated structures
 * NOTE: the hashmap does not retain memory associated to key/value pairs
 *     therefore it will not finalize them as well.
 */
void hashmap_delete(const hashmap_ptr map) {
    if (map) {
        hashmap_map* _map = (hashmap_map*) map;

        if (_map->_buckets)
            free(_map->_buckets);

        if (_map->_keys)
            free(_map->_keys);

        free(_map);
    }
}

/**
 * hashmap_put:
 * puts key/value pair into hashmap if any bucket available
 * NOTE: overwrites any bucket with same key
 */
int hashmap_put(hashmap_ptr map, const any_key_t* key, const any_value_t* value) {
    if (map) {
        int index;
        hashmap_map* _map = (hashmap_map*) map;

        /* find a suitable bucket, if any */
        index = hashmap_hash(_map, key);
        while (HASHMAP_FULL == index) {
            int status = hashmap_rehash(_map);
            if (HASHMAP_OK == status) {
                index = hashmap_hash(_map, key);
            } else {
                return HASHMAP_FULL;
            }
        }

        /* fill the bucket */
        _map->_buckets[index]._key = (any_key_t*) key;
        _map->_buckets[index]._value = (any_value_t*) value;
        _map->_buckets[index]._in_use = 1;
        _map->_cur_size++;
        _map->_changed = 1;

        return HASHMAP_OK;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_get:
 * returns the value associated to a key within the hashmap, if any
 */
int hashmap_get(hashmap_ptr map, const any_key_t* key, any_value_t** value) {
    if (map) {
        int index;
        hashmap_map* _map = (hashmap_map*) map;

        /* find a suitable bucket, if any */
        index = hashmap_find(_map, key);
        if (index < 0) {
            /* key not found, return error code */
            return index;
        }

        /* save value into output parameter */
        *value = _map->_buckets[index]._value;

        return HASHMAP_OK;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_remove:
 * removes key/value pair from hashmap, if found;
 * stores the value in the output variable on return
 */
int hashmap_remove(hashmap_ptr map, const any_key_t* key, any_value_t** value) {
    if (map) {
        int index;
        hashmap_map* _map = (hashmap_map*) map;

        /* find a suitable bucket, if any */
        index = hashmap_find(_map, key);
        if (index < 0) {
            /* key not found, return error code */
            return index;
        }

        /* set bucket as not used */
        _map->_buckets[index]._in_use = 0;
        _map->_cur_size--;
        _map->_changed = 1;
        if (value)
            *value = _map->_buckets[index]._value;

        return HASHMAP_OK;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_keys:
 * returns an array containing all keys in use within the hashmap
 */
int hashmap_keys(hashmap_ptr map, any_key_t*** keys, size_t* size) {
    if (map) {
        size_t i, j;
        hashmap_map* _map = (hashmap_map*) map;

        /* return if hashmap is empty */
        if (_map->_cur_size == 0) {
            return HASHMAP_EMPTY;
        }

        /* if hashmap content changed, refresh keys array */
        if (_map->_changed) {
            /* (re-)allocate memory */
            _map->_keys = (any_key_t**) realloc(_map->_keys,
                          (size_t) (_map->_cur_size * sizeof(any_key_t*)));
            if (!(_map->_keys)) {
                return HASHMAP_ERR;
            }

            /* copy active bucket keys into new array */
            for (i = j = 0; (i < _map->_max_size) &&
                            (j < _map->_cur_size); ++i) {
                if (_map->_buckets[i]._in_use) {
                    _map->_keys[j] = _map->_buckets[i]._key;
                     ++j;
                }
            }
            _map->_changed = 0;
        }

        *keys = _map->_keys;
        *size = _map->_cur_size;
        return HASHMAP_OK;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_iterate:
 * iteratively applies function ``fun'' to each value within the hashmap
 * using ``data'' as additional parameter. Does not check ``fun'' exit code
 */
int hashmap_iterate(hashmap_ptr map, hashmap_fun fun, void* data) {
    return hashmap_iterate_impl(map, fun, data, 0);
}

/**
 * hashmap_iterate_check_ok_status:
 * iteratively applies function ``fun'' to each value within the hashmap
 * using ``data'' as additional parameter. Early stops if ``fun'' exit code
 * is different than HASHMAP_OK (which evaluates to 0).
 */
int hashmap_iterate_check_ok_status(hashmap_ptr map, hashmap_fun fun, void* data) {
    return hashmap_iterate_impl(map, fun, data, 1);
}

/******************************************************************************/
/*** PRIVATE INTERFACE IMPLEMENTATION                                       ***/
/******************************************************************************/

/**
 * hashmap_key_compare_str:
 * compares two '\0'-terminated strings, returning 0 if they are equal
 * NOTE: by default, any_key_t is assumed to be ``char'' and this function
 *     is used to compare two keys for equality.
 */
static inline int hashmap_key_compare_str(const char* key1, const char* key2) {
    return strcmp(key1, key2);
}

/**
 * hashmap_hash_str:
 * computes the hash of a '\0'-terminated string using
 * Bob Jenkins one-at-a-time hash algorithm. [ref: Wikipedia]
 * NOTE: on Arduino we redefined uint32_t as ``unsigned long'';
 */
static inline uint32_t hashmap_key_hash_str(const char* key) {
    uint32_t hash, i;
    size_t len = strlen(key);

    for (hash = i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/**
 * hashmap_hash:
 * returns the index of a bucket in which key/value couple can be
 * stored, if any. Error code otherwise.
 */
static inline int hashmap_hash(hashmap_map* map, const any_key_t* key) {
    if (map) {
        size_t cur_index;
        size_t i;

        /* return if full */
        if (map->_cur_size == map->_max_size) {
            return HASHMAP_FULL;
        }

        /* find best index */
        cur_index = (int) (map->_hash(key) % map->_max_size);

        /* linearly seek usable bucket */
        for (i = 0; i < map->_max_size; ++i) {
            /* bucket unused */
            if (map->_buckets[cur_index]._in_use == 0) {
                return cur_index;
            }
            /* bucket used, same key */
            if (map->_compare(map->_buckets[cur_index]._key, key) == 0 &&
                map->_buckets[cur_index]._in_use == 1) {
                return cur_index;
            }
            /* check next bucket */
            cur_index = (cur_index + 1) % map->_max_size;
        }
        return HASHMAP_FULL;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_rehash:
 * doubles the size of the hashmap, and rehashes all the elements
 * NOTE: this function is particularly expensive, consider initializing
 *     your hashmap to be large enough right from the start
 */
static inline int hashmap_rehash(hashmap_map* map) {
    if (map) {
        size_t old_size, i;
        hashmap_bucket* new_buckets;
        hashmap_bucket* old_buckets;

        /* setup new memory */
        new_buckets = (hashmap_bucket*)
                             calloc(2 * map->_max_size, sizeof(hashmap_bucket));
        if (!new_buckets) {
            /* memory exhausted */
            return HASHMAP_ERR;
        }

        /* update memory */
        old_buckets = map->_buckets;
        map->_buckets = new_buckets;

        /* update size */
        old_size = map->_max_size;
        map->_max_size *= 2;
        map->_cur_size = 0;

        /* rehash the elements */
        for (i = 0; i < old_size; ++i) {
            if (old_buckets[i]._in_use) {
                int status = hashmap_put((hashmap_ptr) map,
                                          old_buckets[i]._key,
                                          old_buckets[i]._value);
                if (status != HASHMAP_OK) {
                    /* corrupted state! */
                    free(old_buckets);
                    return status;
                }
            }
        }

        free(old_buckets);

        return HASHMAP_OK;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_find:
 * returns the index of a bucket in which key/value couple is
 * stored, if any. Error code otherwise.
 */
static inline int hashmap_find(hashmap_map* map, const any_key_t* key) {
    if (map) {
        size_t cur_index;
        size_t i;

        /* return if full */
        if (map->_cur_size == 0) {
            return HASHMAP_EMPTY;
        }

        /* find best index */
        cur_index = (int) (map->_hash(key) % map->_max_size);

        /* linearly seek usable bucket */
        for (i = 0; i < map->_max_size; ++i) {
            /* bucket used, same key: found */
            if (map->_compare(map->_buckets[cur_index]._key, key) == 0 &&
                map->_buckets[cur_index]._in_use == 1) {
                return cur_index;
            }
            /* check next bucket */
            cur_index = (cur_index + 1) % map->_max_size;
        }
        return HASHMAP_NOT_FOUND;
    }
    return HASHMAP_ERR;
}

/**
 * hashmap_iterate_impl:
 * iteratively applies function ``fun'' to each value within the hashmap
 * using ``data'' as additional parameter. When ``check_status'' != 0, early stops
 * if ``fun'' exit code is different than HASHMAP_OK (0).
 */
static inline int hashmap_iterate_impl(hashmap_ptr map,
    hashmap_fun fun, void* data, byte check_status) {
    if (map) {
        size_t i;
        hashmap_map* _map = (hashmap_map*) map;

        /* check if there is anything to be done */
        if (_map->_cur_size == 0) {
            return HASHMAP_OK;
        }

        /* iteratively apply function to active buckets */
        for (i = 0; i < _map->_max_size; ++i) {
            if (_map->_buckets[i]._in_use) {
                int status = fun(data, _map->_buckets[i]._value);
                /* early stop on error */
                if (check_status && status != HASHMAP_OK) {
                    return status;
                }
            }
        }

        return HASHMAP_OK;
    }
    return HASHMAP_ERR;
}
