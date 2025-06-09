/*
 * This code is distributed under the terms of the GNU General Public License.
 * For more information, please refer to the LICENSE file in the root directory.
 * -------------------------------------------------
 * Copyright (C) 2025 Rodrigo R.
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
*/

#ifndef FLUENT_LIBC_HASHMAP_LIBRARY_H
#define FLUENT_LIBC_HASHMAP_LIBRARY_H

// ============= FLUENT LIB C =============
// Typed Hashmap Utility
// ----------------------------------------
// A macro-driven, type-safe hashmap implementation in C.
// Supports:
//
// - Arbitrary key and value types via `DEFINE_HASHMAP`
// - Open-addressing with Robin Hood hashing
// - FNV-1a string hashing and 32-bit integer hashing
// - Tombstones for deletions
// - Automatic resizing (grow factor configurable)
//
// Key Concepts:
// ----------------------------------------
// - `hash_str_key(const char*)`: FNV-1a hash for C-strings
// - `hash_int(const uint32_t*)`: Mixed bitwise hash for uint32_t
// - `hash_probe_distance`: Compute probe distance for Robin Hood logic
// - `hash_entry_status_t`: EMPTY, OCCUPIED, TOMBSTONE states
// - `DEFINE_HASHMAP(K,V,NAME)`: Generates `hashmap_NAME_t` and API
//
// Macro-Generated API (for NAME = foo):
// ----------------------------------------
//   int    hashmap_foo_init (hashmap_foo_t *map, size_t cap, double grow, foo_destructor_t d, hash_fn, cmp_fn);
//   foo_t* hashmap_foo_new  (size_t cap, double grow, foo_destructor_t d, hash_fn, cmp_fn);
//   void   hashmap_foo_free (hashmap_foo_t *map);
//   V*     hashmap_foo_get  (hashmap_foo_t *map, K key);
//   int    hashmap_foo_insert(hashmap_foo_t *map, K key, V value);
//   int    hashmap_foo_remove(hashmap_foo_t *map, K key);
//   int    hashmap_foo_resize(hashmap_foo_t *map, size_t new_cap);
//   hashmap_foo_iter_t      and related `iter_begin` / `iter_next`
//
// Usage Example:
// ----------------------------------------
//     // Define a map from `const char*` to `int` called `string_int`
//     DEFINE_HASHMAP(const char*, int, string_int);
//
//     // Create a new map
//     hashmap_string_int_t *map = hashmap_string_int_new(16, 1.5, NULL, NULL, strcmp);
//     hashmap_string_int_insert(map, "apple", 42);
//     int *v = hashmap_string_int_get(map, "apple");
//     if (v) printf("apple => %d\n", *v);
//     hashmap_string_int_free(map);
//
// ----------------------------------------
// Initial revision: 2025-05-30
// ----------------------------------------
// Old Signatures for reference:
/*
/**
 * @typedef hashmap_destructor_t
 * @brief Function pointer type for a custom destructor for hashmap values.
 *
 * This type defines a function that takes a constant pointer to a hashmap_t structure
 * and performs cleanup or deallocation of resources associated with the hashmap's values.
 * It is intended to be used as a custom destructor callback when freeing the hashmap.
 *
 * @param map Pointer to the hashmap to be destroyed.

typedef void (*hashmap_destructor_t)(const hashmap_t *map);

// ============= HASHMAP FUNCTIONS =============
static inline int hashmap_resize(hashmap_t* map, size_t new_capacity);
static inline int hashmap_insert(hashmap_t* map, void* key, void *value);

// ============= DEFAULT HASHING =============
/**
 * @brief Computes a 32-bit FNV-1a hash for a given null-terminated string key.
 *
 * This function implements the FNV-1a hash algorithm, which is a fast and simple
 * non-cryptographic hash function suitable for hash table lookup. It processes
 * each byte of the input string, applying XOR and multiplication with a prime.
 *
 * @param key The null-terminated string to hash.
 * @return The 32-bit hash value of the input key.

static inline uint32_t hash_str_key(const char *key)
{
    uint32_t hash = 2166136261u; // FNV offset basis
    for (size_t i = 0; key[i]; i++)
    {
        hash ^= (uint8_t)key[i];    // XOR with the current byte
        hash *= 16777619u;          // Multiply by FNV prime
    }
    return hash;
}

/**
 * @brief Hashes a 32-bit integer using a mix of bitwise and multiplicative operations.
 *
 * This function applies a series of XOR and multiplication steps to the input integer
 * to produce a well-distributed 32-bit hash value. It is suitable for use in hash tables
 * and is inspired by techniques such as MurmurHash finalization.
 *
 * @param x_ptr Pointer to a 32-bit integer to hash.
 * @return The hashed 32-bit integer.

static inline uint32_t hash_int(const uint32_t *x_ptr)
{
    uint32_t x = *x_ptr; // Dereference the pointer to get the integer value
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

/**
 * @brief Calculates the probe distance for a given hash and index in the hashmap.
 *
 * This function computes how far an entry at a given index is from its ideal position,
 * as determined by the hash value. It is used in open addressing hashmaps to determine
 * the distance an entry has been displaced from its original hash bucket, which is
 * useful for probing strategies such as Robin Hood hashing.
 *
 * @param hash The hash value of the key.
 * @param index The current index in the hashmap.
 * @param capacity The total capacity of the hashmap.
 * @return The probe distance as a size_t value.

static inline size_t hash_probe_distance
(
    const uint32_t hash,
    const size_t index,
    const size_t capacity
)
{
    return (index + capacity - (hash % capacity)) % capacity;
}

/**
 * @brief Retrieves the value associated with a given key from the hashmap.
 *
 * This function searches for the specified key in the hashmap using open addressing.
 * If the key is found and the entry is occupied, a pointer to the value is returned.
 * If the key is not found or the map/key is NULL, NULL is returned.
 *
 * @param map Pointer to the hashmap structure.
 * @param key Pointer to the key to search for.
 * @return Pointer to the value associated with the key, or NULL if not found.

static inline void* hashmap_get(hashmap_t* map, void* key)
{
    if (!map || !key) return NULL; // Check for NULL map or key
    const uint32_t hash = map->hash_fn(key); // Compute hash for the key
    size_t index = hash % map->capacity;   // Initial index based on hash

    // Probe until an empty slot is found
    while (map->entries[index].status != EMPTY)
    {
        // Check if the entry is occupied, hashes match, and keys are equal
        if (map->entries[index].status == OCCUPIED &&
            map->entries[index].hash == hash &&
            strcmp(map->entries[index].key, key) == 0
        )
        {/**
 * @typedef hashmap_destructor_t
 * @brief Function pointer type for a custom destructor for hashmap values.
 *
 * This type defines a function that takes a constant pointer to a hashmap_t structure
 * and performs cleanup or deallocation of resources associated with the hashmap's values.
 * It is intended to be used as a custom destructor callback when freeing the hashmap.
 *
 * @param map Pointer to the hashmap to be destroyed.

typedef void (*hashmap_destructor_t)(const hashmap_t *map);

// ============= HASHMAP FUNCTIONS =============
static inline int hashmap_resize(hashmap_t* map, size_t new_capacity);
static inline int hashmap_insert(hashmap_t* map, void* key, void *value);

// ============= DEFAULT HASHING =============
/**
 * @brief Computes a 32-bit FNV-1a hash for a given null-terminated string key.
 *
 * This function implements the FNV-1a hash algorithm, which is a fast and simple
 * non-cryptographic hash function suitable for hash table lookup. It processes
 * each byte of the input string, applying XOR and multiplication with a prime.
 *
 * @param key The null-terminated string to hash.
 * @return The 32-bit hash value of the input key.

static inline uint32_t hash_str_key(const char *key)
{
    uint32_t hash = 2166136261u; // FNV offset basis
    for (size_t i = 0; key[i]; i++)
    {
        hash ^= (uint8_t)key[i];    // XOR with the current byte
        hash *= 16777619u;          // Multiply by FNV prime
    }
    return hash;
}

/**
 * @brief Hashes a 32-bit integer using a mix of bitwise and multiplicative operations.
 *
 * This function applies a series of XOR and multiplication steps to the input integer
 * to produce a well-distributed 32-bit hash value. It is suitable for use in hash tables
 * and is inspired by techniques such as MurmurHash finalization.
 *
 * @param x_ptr Pointer to a 32-bit integer to hash.
 * @return The hashed 32-bit integer.

static inline uint32_t hash_int(const uint32_t *x_ptr)
{
    uint32_t x = *x_ptr; // Dereference the pointer to get the integer value
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

/**
 * @brief Calculates the probe distance for a given hash and index in the hashmap.
 *
 * This function computes how far an entry at a given index is from its ideal position,
 * as determined by the hash value. It is used in open addressing hashmaps to determine
 * the distance an entry has been displaced from its original hash bucket, which is
 * useful for probing strategies such as Robin Hood hashing.
 *
 * @param hash The hash value of the key.
 * @param index The current index in the hashmap.
 * @param capacity The total capacity of the hashmap.
 * @return The probe distance as a size_t value.

static inline size_t hash_probe_distance
(
    const uint32_t hash,
    const size_t index,
    const size_t capacity
)
{
    return (index + capacity - (hash % capacity)) % capacity;
}

/**
 * @brief Retrieves the value associated with a given key from the hashmap.
 *
 * This function searches for the specified key in the hashmap using open addressing.
 * If the key is found and the entry is occupied, a pointer to the value is returned.
 * If the key is not found or the map/key is NULL, NULL is returned.
 *
 * @param map Pointer to the hashmap structure.
 * @param key Pointer to the key to search for.
 * @return Pointer to the value associated with the key, or NULL if not found.

static inline void* hashmap_get(hashmap_t* map, void* key)
{
    if (!map || !key) return NULL; // Check for NULL map or key
    const uint32_t hash = map->hash_fn(key); // Compute hash for the key
    size_t index = hash % map->capacity;   // Initial index based on hash

    // Probe until an empty slot is found
    while (map->entries[index].status != EMPTY)
    {
        // Check if the entry is occupied, hashes match, and keys are equal
        if (map->entries[index].status == OCCUPIED &&
            map->entries[index].hash == hash &&
            strcmp(map->entries[index].key, key) == 0
        )
        {
            // Return pointer to the value if found
            return map->entries[index].value;
        }

        // Move to the next index (linear probing)
        index = (index + 1) % map->capacity;
    }
    // Key not found
    return NULL;
}

/**
 * @brief Initializes a hashmap structure with the specified parameters.
 *
 * Allocates and zero-initializes the entries array for the hashmap.
 * Sets the initial capacity, count, grow factor, destructor, hash function,
 * and key-freeing flag. If allocation fails, frees the map and returns 0.
 *
 * @param map Pointer to the hashmap structure to initialize.
 * @param capacity Initial number of slots in the hashmap.
 * @param grow_factor Factor by which the hashmap grows when resized.
 * @param destructor Function pointer for a custom destructor to free values.
 * @param hash_fn Function pointer for a custom hash function (uses default if NULL).
 * @param free_keys Flag indicating if keys should be freed on destruction (1 to free, 0 to not free).
 * @return 1 on success, 0 on failure.

static inline int hashmap_init(
    hashmap_t *map,
    const size_t capacity,
    const double grow_factor,
    const hashmap_destructor_t destructor,
    const hash_function_t hash_fn,
    const int free_keys
)
{
    if (!map) return 0;

    map->entries = calloc(capacity, sizeof(hash_entry_t));
    if (!map->entries)
    {
        return 0; // Return failure if memory allocation fails
    }

    map->capacity = capacity;
    map->count = 0;
    map->grow_factor = grow_factor;
    map->destructor = destructor;
    map->hash_fn = hash_fn ? hash_fn : (hash_function_t)hash_str_key; // Use default hash function if none provided
    map->free_keys = free_keys; // Set the flag for freeing keys
    return 1; // Return success
}

/**
 * @brief Allocates and initializes a new hashmap with the specified capacity and element size.
 *
 * This function creates a new hashmap_t structure, allocates memory for the entries array,
 * and initializes its fields. The entries array is zero-initialized.
 *
 * @param capacity The initial number of slots in the hashmap.
 * @param grow_factor The factor by which the hashmap should grow when resized.
 * @param destructor Function pointer for a custom destructor to free values.
 * @param hash_fn Function pointer for a custom hash function.
 * @param free_keys Flag indicating if keys should be freed on destruction (1 to free, 0 to not free).
 * @return Pointer to the newly created hashmap_t, or NULL on allocation failure.

static inline hashmap_t* hashmap_new(
    const size_t capacity,
    const double grow_factor,
    const hashmap_destructor_t destructor,
    const hash_function_t hash_fn,
    const int free_keys
)
{
    hashmap_t* map = (hashmap_t*)malloc(sizeof(hashmap_t));
    if (!map) return NULL; // Return NULL if memory allocation fails

    if (!hashmap_init(
        map,
        capacity,
        grow_factor,
        destructor,
        hash_fn,
        free_keys
    ))
    {
        free(map); // Free the map if initialization fails
        return NULL; // Return NULL on failure
    }

    return map;
}

/**
 * @brief Frees all memory associated with the given hashmap.
 *
 * This function releases the memory allocated for the hashmap structure,
 * including the entries array itself.
 * If the map pointer is NULL, the function does nothing.
 *
 * @param map Pointer to the hashmap to free.

static inline void hashmap_free(hashmap_t* map)
{
    if (!map) return;
    // Call the destructor if one is set
    if (map->destructor)
    {
        map->destructor(map);
    }

    // Free all keys in the entries array
    if (map->free_keys)
    {
        for (size_t i = 0; i < map->capacity; i++)
        {
            if (map->entries[i].status == OCCUPIED)
            {
                free(map->entries[i].key);
            }
        }
    }

    free(map->entries);
    free(map);
}

/**
 * @brief Inserts a key-value pair into the hashmap using Robin Hood hashing.
 *
 * This function inserts a new key-value pair into the hashmap. If the load factor exceeds 90%,
 * the hashmap is resized to double its current capacity. The function uses Robin Hood hashing
 * to minimize probe sequence variance, swapping entries if the new entry has a greater probe
 * distance than the existing entry at the current index.
 *
 * @param map Pointer to the hashmap structure.
 * @param key Pointer to the key to insert (should be a cloned value)
 * @param value Pointer to the value to associate with the key.
 * @return 1 on successful insertion, 0 on failure (e.g., allocation failure or invalid input).

static inline int hashmap_insert(hashmap_t* map, void* key, void *value)
{
    if (!map || !key) return 0;
    if (map->count + 1 > map->capacity * 0.9)
    {
        if (!hashmap_resize(map, map->capacity * map->grow_factor)) return 0;
    }

    const uint32_t hash = map->hash_fn(key);
    size_t index = hash % map->capacity;
    size_t dist = 0;
    hash_entry_t new_entry = {
        .key = key,
        .value = value,
        .status = OCCUPIED,
        .hash = hash
    };
    if (!new_entry.key) return 0;

    while (1)
    {
        if (map->entries[index].status == EMPTY || map->entries[index].status == TOMBSTONE)
        {
            map->entries[index] = new_entry;
            map->count++;
            return 1;
        }

        // Robin Hood hashing: compare probe distances
        const size_t existing_dist = hash_probe_distance(map->entries[index].hash, index, map->capacity);
        if (dist > existing_dist)
        {
            // Swap with existing entry
            const hash_entry_t temp = map->entries[index];
            map->entries[index] = new_entry;
            new_entry = temp;
            dist = existing_dist;
        }

        index = (index + 1) % map->capacity;
        dist++;
    }
}

/**
 * @brief Resizes the hashmap to a new capacity.
 *
 * This function creates a new hashmap with the specified new capacity and reinserts all
 * existing OCCUPIED entries from the old map into the new one. It then frees the memory
 * associated with the old entries and updates the original map to use the new entries array.
 * If memory allocation fails at any point, the function returns 0 to indicate failure.
 *
 * @param map Pointer to the hashmap to resize.
 * @param new_capacity The desired new capacity for the hashmap.
 * @return 1 on success, 0 on failure.

static inline int hashmap_resize(hashmap_t* map, const size_t new_capacity)
{
    hashmap_t* new_map = hashmap_new(new_capacity, map->grow_factor, map->destructor, map->hash_fn, map->free_keys);
    if (!new_map) return 0;

    // Reinsert all existing entries
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i].status == OCCUPIED)
        {
            if (
                !hashmap_insert(
                    new_map,
                    map->entries[i].key,
                    map->entries[i].value
                )
            )
            {
                hashmap_free(new_map);
                return 0;
            }
        }
    }

    // Free old entries and update map
    if (map->free_keys)
    {
        for (size_t i = 0; i < map->capacity; i++)
        {
            if (map->entries[i].status == OCCUPIED)
            {
                free(map->entries[i].key);
            }
        }
    }

    free(map->entries);
    map->entries = new_map->entries;
    map->capacity = new_map->capacity;
    map->count = new_map->count;
    free(new_map);
    return 1;
}

/**
 * @brief Removes a key-value pair from the hashmap.
 *
 * This function searches for the specified key in the hashmap using open addressing.
 * If the key is found and the entry is occupied, it frees the memory for the key,
 * marks the entry as a tombstone, decrements the count, and returns 1.
 * If the key is not found or the map/key is NULL, it returns 0.
 *
 * @param map Pointer to the hashmap structure.
 * @param key Pointer to the key to remove.
 * @return 1 if the key was found and removed, 0 otherwise.

static inline int hashmap_remove(hashmap_t* map, void* key)
{
    if (!map || !key) return 0;
    const uint32_t hash = map->hash_fn(key);
    size_t index = hash % map->capacity;

    while (map->entries[index].status != EMPTY)
    {
        if (
            map->entries[index].status == OCCUPIED &&
            map->entries[index].hash == hash &&
            strcmp(map->entries[index].key, key) == 0
        )
        {
            if (map->free_keys)
            {
                free(map->entries[index].key);
            }

            map->entries[index].key = NULL;
            map->entries[index].status = TOMBSTONE;
            map->count--;
            return 1;
        }

        index = (index + 1) % map->capacity;
    }
    return 0;
}

/**
 * @brief Initializes a hashmap iterator at the beginning of the map.
 *
 * This function creates a new iterator for the given hashmap, starting at index 0.
 * The iterator can be used with `hashmap_iter_next` to traverse all occupied entries.
 *
 * @param map Pointer to the hashmap to iterate over.
 * @return An initialized hashmap_iter_t structure.

static inline hashmap_iter_t hashmap_iter_begin(hashmap_t* map)
{
    const hashmap_iter_t iter = { .map = map, .index = 0 };
    return iter;
}

/**
 * @brief Advances the iterator to the next occupied entry in the hashmap.
 *
 * This function iterates through the hashmap entries starting from the current index
 * of the iterator. It returns a pointer to the next OCCUPIED entry, or NULL if there
 * are no more occupied entries.
 *
 * @param iter Pointer to the hashmap iterator.
 * @return Pointer to the next occupied hash_entry_t, or NULL if iteration is complete.

static inline hash_entry_t* hashmap_iter_next(hashmap_iter_t* iter)
{
    if (!iter || !iter->map) return NULL; // Check for valid iterator and map

    // Iterate through the entries array
    while (iter->index < iter->map->capacity)
    {
        hash_entry_t* entry = &iter->map->entries[iter->index++];
        if (entry->status == OCCUPIED)
        {
            return entry; // Return pointer to the next occupied entry
        }
    }

    return NULL; // No more occupied entries
}
            // Return pointer to the value if found
            return map->entries[index].value;
        }

        // Move to the next index (linear probing)
        index = (index + 1) % map->capacity;
    }
    // Key not found
    return NULL;
}

/**
 * @brief Initializes a hashmap structure with the specified parameters.
 *
 * Allocates and zero-initializes the entries array for the hashmap.
 * Sets the initial capacity, count, grow factor, destructor, hash function,
 * and key-freeing flag. If allocation fails, frees the map and returns 0.
 *
 * @param map Pointer to the hashmap structure to initialize.
 * @param capacity Initial number of slots in the hashmap.
 * @param grow_factor Factor by which the hashmap grows when resized.
 * @param destructor Function pointer for a custom destructor to free values.
 * @param hash_fn Function pointer for a custom hash function (uses default if NULL).
 * @param free_keys Flag indicating if keys should be freed on destruction (1 to free, 0 to not free).
 * @return 1 on success, 0 on failure.

static inline int hashmap_init(
    hashmap_t *map,
    const size_t capacity,
    const double grow_factor,
    const hashmap_destructor_t destructor,
    const hash_function_t hash_fn,
    const int free_keys
)
{
    if (!map) return 0;

    map->entries = calloc(capacity, sizeof(hash_entry_t));
    if (!map->entries)
    {
        return 0; // Return failure if memory allocation fails
    }

    map->capacity = capacity;
    map->count = 0;
    map->grow_factor = grow_factor;
    map->destructor = destructor;
    map->hash_fn = hash_fn ? hash_fn : (hash_function_t)hash_str_key; // Use default hash function if none provided
    map->free_keys = free_keys; // Set the flag for freeing keys
    return 1; // Return success
}

/**
 * @brief Allocates and initializes a new hashmap with the specified capacity and element size.
 *
 * This function creates a new hashmap_t structure, allocates memory for the entries array,
 * and initializes its fields. The entries array is zero-initialized.
 *
 * @param capacity The initial number of slots in the hashmap.
 * @param grow_factor The factor by which the hashmap should grow when resized.
 * @param destructor Function pointer for a custom destructor to free values.
 * @param hash_fn Function pointer for a custom hash function.
 * @param free_keys Flag indicating if keys should be freed on destruction (1 to free, 0 to not free).
 * @return Pointer to the newly created hashmap_t, or NULL on allocation failure.

static inline hashmap_t* hashmap_new(
    const size_t capacity,
    const double grow_factor,
    const hashmap_destructor_t destructor,
    const hash_function_t hash_fn,
    const int free_keys
)
{
    hashmap_t* map = (hashmap_t*)malloc(sizeof(hashmap_t));
    if (!map) return NULL; // Return NULL if memory allocation fails

    if (!hashmap_init(
        map,
        capacity,
        grow_factor,
        destructor,
        hash_fn,
        free_keys
    ))
    {
        free(map); // Free the map if initialization fails
        return NULL; // Return NULL on failure
    }

    return map;
}

/**
 * @brief Frees all memory associated with the given hashmap.
 *
 * This function releases the memory allocated for the hashmap structure,
 * including the entries array itself.
 * If the map pointer is NULL, the function does nothing.
 *
 * @param map Pointer to the hashmap to free.

static inline void hashmap_free(hashmap_t* map)
{
    if (!map) return;
    // Call the destructor if one is set
    if (map->destructor)
    {
        map->destructor(map);
    }

    // Free all keys in the entries array
    if (map->free_keys)
    {
        for (size_t i = 0; i < map->capacity; i++)
        {
            if (map->entries[i].status == OCCUPIED)
            {
                free(map->entries[i].key);
            }
        }
    }

    free(map->entries);
    free(map);
}

/**
 * @brief Inserts a key-value pair into the hashmap using Robin Hood hashing.
 *
 * This function inserts a new key-value pair into the hashmap. If the load factor exceeds 90%,
 * the hashmap is resized to double its current capacity. The function uses Robin Hood hashing
 * to minimize probe sequence variance, swapping entries if the new entry has a greater probe
 * distance than the existing entry at the current index.
 *
 * @param map Pointer to the hashmap structure.
 * @param key Pointer to the key to insert (should be a cloned value)
 * @param value Pointer to the value to associate with the key.
 * @return 1 on successful insertion, 0 on failure (e.g., allocation failure or invalid input).

static inline int hashmap_insert(hashmap_t* map, void* key, void *value)
{
    if (!map || !key) return 0;
    if (map->count + 1 > map->capacity * 0.9)
    {
        if (!hashmap_resize(map, map->capacity * map->grow_factor)) return 0;
    }

    const uint32_t hash = map->hash_fn(key);
    size_t index = hash % map->capacity;
    size_t dist = 0;
    hash_entry_t new_entry = {
        .key = key,
        .value = value,
        .status = OCCUPIED,
        .hash = hash
    };
    if (!new_entry.key) return 0;

    while (1)
    {
        if (map->entries[index].status == EMPTY || map->entries[index].status == TOMBSTONE)
        {
            map->entries[index] = new_entry;
            map->count++;
            return 1;
        }

        // Robin Hood hashing: compare probe distances
        const size_t existing_dist = hash_probe_distance(map->entries[index].hash, index, map->capacity);
        if (dist > existing_dist)
        {
            // Swap with existing entry
            const hash_entry_t temp = map->entries[index];
            map->entries[index] = new_entry;
            new_entry = temp;
            dist = existing_dist;
        }

        index = (index + 1) % map->capacity;
        dist++;
    }
}

/**
 * @brief Resizes the hashmap to a new capacity.
 *
 * This function creates a new hashmap with the specified new capacity and reinserts all
 * existing OCCUPIED entries from the old map into the new one. It then frees the memory
 * associated with the old entries and updates the original map to use the new entries array.
 * If memory allocation fails at any point, the function returns 0 to indicate failure.
 *
 * @param map Pointer to the hashmap to resize.
 * @param new_capacity The desired new capacity for the hashmap.
 * @return 1 on success, 0 on failure.

static inline int hashmap_resize(hashmap_t* map, const size_t new_capacity)
{
    hashmap_t* new_map = hashmap_new(new_capacity, map->grow_factor, map->destructor, map->hash_fn, map->free_keys);
    if (!new_map) return 0;

    // Reinsert all existing entries
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i].status == OCCUPIED)
        {
            if (
                !hashmap_insert(
                    new_map,
                    map->entries[i].key,
                    map->entries[i].value
                )
            )
            {
                hashmap_free(new_map);
                return 0;
            }
        }
    }

    // Free old entries and update map
    if (map->free_keys)
    {
        for (size_t i = 0; i < map->capacity; i++)
        {
            if (map->entries[i].status == OCCUPIED)
            {
                free(map->entries[i].key);
            }
        }
    }

    free(map->entries);
    map->entries = new_map->entries;
    map->capacity = new_map->capacity;
    map->count = new_map->count;
    free(new_map);
    return 1;
}

/**
 * @brief Removes a key-value pair from the hashmap.
 *
 * This function searches for the specified key in the hashmap using open addressing.
 * If the key is found and the entry is occupied, it frees the memory for the key,
 * marks the entry as a tombstone, decrements the count, and returns 1.
 * If the key is not found or the map/key is NULL, it returns 0.
 *
 * @param map Pointer to the hashmap structure.
 * @param key Pointer to the key to remove.
 * @return 1 if the key was found and removed, 0 otherwise.

static inline int hashmap_remove(hashmap_t* map, void* key)
{
    if (!map || !key) return 0;
    const uint32_t hash = map->hash_fn(key);
    size_t index = hash % map->capacity;

    while (map->entries[index].status != EMPTY)
    {
        if (
            map->entries[index].status == OCCUPIED &&
            map->entries[index].hash == hash &&
            strcmp(map->entries[index].key, key) == 0
        )
        {
            if (map->free_keys)
            {
                free(map->entries[index].key);
            }

            map->entries[index].key = NULL;
            map->entries[index].status = TOMBSTONE;
            map->count--;
            return 1;
        }

        index = (index + 1) % map->capacity;
    }
    return 0;
}

/**
 * @brief Initializes a hashmap iterator at the beginning of the map.
 *
 * This function creates a new iterator for the given hashmap, starting at index 0.
 * The iterator can be used with `hashmap_iter_next` to traverse all occupied entries.
 *
 * @param map Pointer to the hashmap to iterate over.
 * @return An initialized hashmap_iter_t structure.

static inline hashmap_iter_t hashmap_iter_begin(hashmap_t* map)
{
    const hashmap_iter_t iter = { .map = map, .index = 0 };
    return iter;
}

/**
 * @brief Advances the iterator to the next occupied entry in the hashmap.
 *
 * This function iterates through the hashmap entries starting from the current index
 * of the iterator. It returns a pointer to the next OCCUPIED entry, or NULL if there
 * are no more occupied entries.
 *
 * @param iter Pointer to the hashmap iterator.
 * @return Pointer to the next occupied hash_entry_t, or NULL if iteration is complete.

static inline hash_entry_t* hashmap_iter_next(hashmap_iter_t* iter)
{
    if (!iter || !iter->map) return NULL; // Check for valid iterator and map

    // Iterate through the entries array
    while (iter->index < iter->map->capacity)
    {
        hash_entry_t* entry = &iter->map->entries[iter->index++];
        if (entry->status == OCCUPIED)
        {
            return entry; // Return pointer to the next occupied entry
        }
    }

    return NULL; // No more occupied entries
}
*/

// ============= FLUENT LIB C++ =============
#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// ============= DEFAULT HASHING =============
/**
 * @brief Computes a 32-bit FNV-1a hash for a given null-terminated string key.
 *
 * This function implements the FNV-1a hash algorithm, which is a fast and simple
 * non-cryptographic hash function suitable for hash table lookup. It processes
 * each byte of the input string, applying XOR and multiplication with a prime.
 *
 * @param key The null-terminated string to hash.
 * @return The 32-bit hash value of the input key.
 */
static inline uint32_t hash_str_key(const char *key)
{
    uint32_t hash = 2166136261u; // FNV offset basis
    for (size_t i = 0; key[i]; i++)
    {
        hash ^= (uint8_t)key[i];    // XOR with the current byte
        hash *= 16777619u;          // Multiply by FNV prime
    }
    return hash;
}

/**
 * @brief Computes a 32-bit FNV-1a hash for a single character key.
 *
 * This function applies the FNV-1a hash algorithm to a single character,
 * producing a 32-bit hash value suitable for use as a hash table key.
 *
 * @param c The character to hash.
 * @return The 32-bit hash value of the input character.
 */
static inline uint32_t hash_char_key(const char c)
{
    uint32_t hash = 2166136261u;   // FNV offset basis
    hash ^= (uint8_t)c;           // XOR with the character
    hash *= 16777619u;            // Multiply by FNV prime
    return hash;
}

/**
 * @brief Hashes a 32-bit integer using a mix of bitwise and multiplicative operations.
 *
 * This function applies a series of XOR and multiplication steps to the input integer
 * to produce a well-distributed 32-bit hash value. It is suitable for use in hash tables
 * and is inspired by techniques such as MurmurHash finalization.
 *
 * @param x The 32-bit integer to hash.
 * @return The hashed 32-bit integer.
 */
static inline uint32_t hash_int(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

/**
 * @brief Calculates the probe distance for a given hash and index in the hashmap.
 *
 * This function computes how far an entry at a given index is from its ideal position,
 * as determined by the hash value. It is used in open addressing hashmaps to determine
 * the distance an entry has been displaced from its original hash bucket, which is
 * useful for probing strategies such as Robin Hood hashing.
 *
 * @param hash The hash value of the key.
 * @param index The current index in the hashmap.
 * @param capacity The total capacity of the hashmap.
 * @return The probe distance as a size_t value.
 */
static inline size_t hash_probe_distance
(
    const uint32_t hash,
    const size_t index,
    const size_t capacity
)
{
    return (index + capacity - (hash % capacity)) % capacity;
}

/**
 * @enum hash_entry_status_t
 * @brief Represents the status of a hashmap entry.
 *
 * This enumeration defines the possible states for an entry in the hashmap:
 * - __FLUENT_LIBC_HASHMAP_EMPTY: The entry is unused.
 * - OCCUPIED: The entry contains a valid key-value pair.
 * - TOMBSTONE: The entry was previously occupied but has been deleted.
 */
static enum hash_entry_status_t
{
    __FLUENT_LIBC_HASHMAP_EMPTY = 0, ///< Indicates that the entry is empty and not used
    OCCUPIED, ///< Indicates that the entry is occupied with a valid key-value pair
    TOMBSTONE ///< Indicates that the entry was previously occupied but has been deleted
};
typedef enum hash_entry_status_t hash_entry_status_t;

// ============= TYPED HASHMAP MACRO =============
#define DEFINE_HASHMAP(K, V, NAME)                             \
    typedef unsigned long (*hash_##NAME##_function_t)(const K key);     \
    typedef int (*hash_##NAME##_cmp_t)(const K key, const K key2);      \
    typedef struct                                                      \
    {                                                                   \
        K key;                                                          \
        V value;                                                        \
        hash_entry_status_t status;                                     \
        uint32_t hash;                                                  \
    } hash_##NAME##_entry_t;                                            \
    typedef struct hashmap_##NAME##_t                                   \
    {                                                                   \
        hash_##NAME##_entry_t *entries;                                 \
        size_t capacity;                                                \
        size_t count;                                                   \
        double grow_factor;                                             \
        void (*destructor)                                              \
            (const struct hashmap_##NAME##_t *map);                     \
        hash_##NAME##_function_t hash_fn;                               \
        hash_##NAME##_cmp_t cmp_fn;                                     \
    } hashmap_##NAME##_t;                                               \
    typedef void (*hash_##NAME##_destructor_t)(const hashmap_##NAME##_t *map); \
    static inline int hashmap_##NAME##_resize(hashmap_##NAME##_t* map, size_t new_capacity); \
    static inline int hashmap_##NAME##_insert(hashmap_##NAME##_t* map, K key, V value); \
    typedef struct                                                      \
    {                                                                   \
        hashmap_##NAME##_t *map;                                        \
        size_t index;                                                   \
    } hashmap_##NAME##_iter_t;                                          \
    static inline int hashmap_##NAME##_init(                            \
        hashmap_##NAME##_t *map,                                        \
        const size_t capacity,                                          \
        const double grow_factor,                                       \
        const hash_##NAME##_destructor_t destructor,                 \
        const hash_##NAME##_function_t hash_fn,                      \
        const hash_##NAME##_cmp_t cmp_fn                             \
    )                                                                   \
    {                                                                   \
        if (!map) return 0;                                             \
        map->entries = calloc(capacity, sizeof(hash_##NAME##_entry_t)); \
        if (!map->entries)                                              \
        {                                                               \
            return 0;                                                   \
        }                                                               \
        map->capacity = capacity;                                       \
        map->count = 0;                                                 \
        map->grow_factor = grow_factor;                                 \
        map->destructor = destructor;                                   \
        map->hash_fn = hash_fn ? hash_fn : (hash_##NAME##_function_t)hash_str_key; \
        map->cmp_fn = cmp_fn;                                           \
        return 1;                                                       \
    }                                                                   \
    static inline hashmap_##NAME##_t *hashmap_##NAME##_new(             \
        const size_t capacity,                                          \
        const double grow_factor,                                       \
        const hash_##NAME##_destructor_t destructor,                 \
        const hash_##NAME##_function_t hash_fn,                      \
        const hash_##NAME##_cmp_t cmp_fn                             \
    )                                                                   \
    {                                                                   \
        hashmap_##NAME##_t* map = (hashmap_##NAME##_t*)malloc(sizeof(hashmap_##NAME##_t)); \
        if (!map) return NULL;                                          \
        if (!hashmap_##NAME##_init(                                     \
            map,                                                        \
            capacity,                                                   \
            grow_factor,                                                \
            destructor,                                                 \
            hash_fn,                                                    \
            cmp_fn                                                      \
        ))                                                              \
        {                                                               \
            free(map);                                                  \
            return NULL;                                                \
        }                                                               \
        return map;                                                     \
    }                                                                   \
    static inline void hashmap_##NAME##_free(hashmap_##NAME##_t *map)  \
    {                                                                   \
        if (!map) return;                                               \
        if (map->destructor)                                            \
        {                                                               \
            map->destructor(map);                                       \
        }                                                               \
        free(map->entries);                                             \
        free(map);                                                      \
    }                                                                   \
    static inline V *hashmap_##NAME##_get(hashmap_##NAME##_t *map, K key) \
    {                                                                   \
        if (!map) return NULL;                                          \
        const uint32_t hash = map->hash_fn(key);                        \
        size_t index = hash % map->capacity;                            \
        while (map->entries[index].status != __FLUENT_LIBC_HASHMAP_EMPTY) \
        {                                                               \
            if (                                                        \
                map->entries[index].status == OCCUPIED &&               \
                map->entries[index].hash == hash &&                     \
                map->cmp_fn(map->entries[index].key, key)               \
            )                                                           \
            {                                                           \
                return &map->entries[index].value;                      \
            }                                                           \
            index = (index + 1) % map->capacity;                        \
        }                                                               \
        return NULL;                                                    \
    }                                                                   \
    static inline int hashmap_##NAME##_insert(hashmap_##NAME##_t *map, K key, V value) \
    {                                                                   \
        if (!map) return 0;                                             \
        if (map->count + 1 > map->capacity * 0.9)                       \
        {                                                               \
            if (!hashmap_##NAME##_resize(map, map->capacity * map->grow_factor)) return 0; \
        }                                                               \
        const uint32_t hash = map->hash_fn(key);                        \
        size_t index = hash % map->capacity;                            \
        size_t dist = 0;                                                \
        hash_##NAME##_entry_t new_entry = {                             \
            .key = key,                                                 \
            .value = value,                                             \
            .status = OCCUPIED,                                         \
            .hash = hash                                                \
        };                                                              \
        while (1)                                                       \
        {                                                               \
            if (map->entries[index].status == __FLUENT_LIBC_HASHMAP_EMPTY || map->entries[index].status == TOMBSTONE) \
            {                                                           \
                map->entries[index] = new_entry;                        \
                map->count++;                                           \
                return 1;                                               \
            }                                                           \
            const size_t existing_dist = hash_probe_distance(map->entries[index].hash, index, map->capacity); \
            if (dist > existing_dist)                                   \
            {                                                           \
                const hash_##NAME##_entry_t temp = map->entries[index]; \
                map->entries[index] = new_entry;                        \
                new_entry = temp;                                       \
                dist = existing_dist;                                   \
            }                                                           \
            index = (index + 1) % map->capacity;                        \
            dist++;                                                     \
        }                                                               \
    }                                                                   \
    static inline int hashmap_##NAME##_resize(hashmap_##NAME##_t *map, const size_t new_capacity) \
    {                                                                   \
        hashmap_##NAME##_t* new_map = hashmap_##NAME##_new(new_capacity, map->grow_factor, map->destructor, map->hash_fn, map->cmp_fn); \
        if (!new_map) return 0;                                         \
        for (size_t i = 0; i < map->capacity; i++)                      \
        {                                                               \
            if (map->entries[i].status == OCCUPIED)                     \
            {                                                           \
                if (                                                    \
                    !hashmap_##NAME##_insert(                           \
                        new_map,                                        \
                        map->entries[i].key,                            \
                        map->entries[i].value                           \
                    )                                                   \
                )                                                       \
                {                                                       \
                    hashmap_##NAME##_free(new_map);                     \
                    return 0;                                           \
                }                                                       \
            }                                                           \
        }                                                               \
        free(map->entries);                                             \
        map->entries = new_map->entries;                                \
        map->capacity = new_map->capacity;                              \
        map->count = new_map->count;                                    \
        free(new_map);                                                  \
        return 1;                                                       \
    }                                                                   \
    static inline int hashmap_##NAME##_remove(hashmap_##NAME##_t *map, const K key) \
    {                                                                   \
        if (!map) return 0;                                             \
        const uint32_t hash = map->hash_fn(key);                        \
        size_t index = hash % map->capacity;                            \
        while (map->entries[index].status != __FLUENT_LIBC_HASHMAP_EMPTY) \
        {                                                               \
            if (                                                        \
                map->entries[index].status == OCCUPIED &&               \
                map->entries[index].hash == hash &&                     \
                map->cmp_fn(map->entries[index].key, key)               \
            )                                                           \
            {                                                           \
                map->entries[index].key = (K){0};                       \
                map->entries[index].status = TOMBSTONE;                 \
                map->count--;                                           \
                return 1;                                               \
            }                                                           \
            index = (index + 1) % map->capacity;                        \
        }                                                               \
        return 0;                                                       \
    }                                                                   \
    static inline hashmap_##NAME##_iter_t hashmap_##NAME##_iter_begin(hashmap_##NAME##_t *map) \
    {                                                                   \
        const hashmap_##NAME##_iter_t iter = { .map = map, .index = 0 }; \
        return iter;                                                    \
    }                                                                   \
    static inline hash_##NAME##_entry_t *hashmap_##NAME##_iter_next(hashmap_##NAME##_iter_t *iter) \
    {                                                                   \
        if (!iter || !iter->map) return NULL;                           \
        while (iter->index < iter->map->capacity)                       \
        {                                                               \
            hash_##NAME##_entry_t *entry = &iter->map->entries[iter->index++]; \
            if (entry->status == OCCUPIED)                              \
            {                                                           \
                return entry;                                           \
            }                                                           \
        }                                                               \
        return NULL;                                                    \
    }

// ============= FLUENT LIB C++ =============
#if defined(__cplusplus)
}
#endif

#endif //FLUENT_LIBC_HASHMAP_LIBRARY_H