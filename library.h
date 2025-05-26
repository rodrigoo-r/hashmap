/*
 * This code is distributed under the terms of the GNU General Public License.
 * For more information, please refer to the LICENSE file in the root directory.
 * -------------------------------------------------
 * Copyright (C) 2025 Rodrigo R.
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
*/

// ============= FLUENT LIB C =============
// Hashmap Library Header File
// ----------------------------------------
// A data structure for efficient key-value pair storage and retrieval.
// Implemented in C with open addressing and Robin Hood hashing.
// ----------------------------------------
// Hashing: Uses FNV-1a (see `hash_key_t`) for keys by default.
//
// Function Signatures:
// ----------------------------------------
// hashmap_t* hashmap_new(size_t capacity, size_t el_size, double grow_factor);
// int hashmap_insert(hashmap_t* map, const char* key, void* value);
// void* hashmap_get(hashmap_t* map, const char* key);
// int hashmap_resize(hashmap_t* map, size_t new_capacity);
// void hashmap_free(hashmap_t* map);
// int hashmap_remove(hashmap_t* map, const char* key);
//
// Example Usage:
// ----------------------------------------
// hashmap_t* map = hashmap_new(16, sizeof(int), 2.0);
// int value = 42;
// hashmap_insert(map, "answer", &value);
// int* found = (int*)hashmap_get(map, "answer");
// if (found) printf("Found: %d\n", *found);
// hashmap_free(map);
// hashmap_remove(map, "answer");
// ----------------------------------------
// Initial revision: 2025-05-21
// ----------------------------------------

#ifndef HASHMAP_LIBRARY_H
#define HASHMAP_LIBRARY_H

// ============= FLUENT LIB C++ =============
#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * @enum hash_entry_status_t
 * @brief Represents the status of a hashmap entry.
 *
 * This enumeration defines the possible states for an entry in the hashmap:
 * - EMPTY: The entry is unused.
 * - OCCUPIED: The entry contains a valid key-value pair.
 * - TOMBSTONE: The entry was previously occupied but has been deleted.
 */
typedef enum
{
    EMPTY = 0, ///< Indicates that the entry is empty and not used
    OCCUPIED, ///< Indicates that the entry is occupied with a valid key-value pair
    TOMBSTONE ///< Indicates that the entry was previously occupied but has been deleted
} hash_entry_status_t;

/**
 * @struct hash_entry_t
 * @brief Represents a single key-value pair in the hashmap.
 *
 * This structure stores a key as a void pointer (typically a null-terminated string)
 * The value can be of any type, as it is represented by a void pointer.
 */
typedef struct
{
    void *key;      ///< Key for the hashmap entry (generic pointer, typically a string)
    void *value;    ///< Value associated with the key (generic pointer)
    hash_entry_status_t status; ///< Status of the entry (EMPTY, OCCUPIED, TOMBSTONE)
    uint32_t hash; ///< Hash value of the key, used for quick lookups
} hash_entry_t;

/**
 * @struct hashmap_t
 * @brief Represents a hash map data structure for key-value storage.
 *
 * This structure contains an array of hash entries, the total capacity of the map,
 * and the current number of key-value pairs stored.
 *
 * - entries: Pointer to the array of hash entries.
 * - capacity: Total number of slots in the hash map.
 * - count: Number of key-value pairs currently stored.
 */
typedef struct
{
    hash_entry_t* entries; ///< Array of hash entries
    size_t capacity;       ///< Total capacity of the hash map
    size_t count;          ///< Number of key-value pairs stored
    size_t el_size;       ///< Size of each entry in bytes
    double grow_factor;  ///< Growth factor for resizing the hashmap
} hashmap_t;

/**
 * @struct hashmap_iter_t
 * @brief Iterator for traversing a hashmap.
 */
typedef struct {
    hashmap_t* map;    ///< Pointer to the hashmap
    size_t index;      ///< Current index in the entries array
} hashmap_iter_t;

/**
 * @typedef hash_function_t
 * @brief Function pointer type for hashing a string key.
 *
 * This type defines a function that takes a constant character pointer (the key)
 * and returns an unsigned long hash value. Used to specify custom hash functions
 * for the hashmap.
 */
typedef unsigned long (*hash_function_t)(const void *key);

// ============= HASHMAP FUNCTIONS =============
int hashmap_resize(hashmap_t* map, size_t new_capacity);
int hashmap_insert(hashmap_t* map, const char* key, void *value);
void hashmap_free(hashmap_t* map);
void* hashmap_get(hashmap_t* map, const char* key);
hashmap_t* hashmap_new(size_t capacity, size_t el_size, double grow_factor);
int hashmap_remove(hashmap_t* map, const char* key);

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
inline uint32_t hash_str_key(const char* key)
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
 * @param x The 32-bit integer to hash.
 * @return The hashed 32-bit integer.
 */
inline uint32_t hash_int(uint32_t x)
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
inline size_t hash_probe_distance_t
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
 * @param hash_fn Function pointer for a custom hash function.
 * @return Pointer to the value associated with the key, or NULL if not found.
 */
inline void* hashmap_get(hashmap_t* map, void* key, const hash_function_t hash_fn)
{
    if (!map || !key) return NULL; // Check for NULL map or key
    const uint32_t hash = hash_fn(key); // Compute hash for the key
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
            return &map->entries[index].value;
        }

        // Move to the next index (linear probing)
        index = (index + 1) % map->capacity;
    }
    // Key not found
    return NULL;
}

/**
 * @brief Allocates and initializes a new hashmap with the specified capacity and element size.
 *
 * This function creates a new hashmap_t structure, allocates memory for the entries array,
 * and initializes its fields. The entries array is zero-initialized.
 *
 * @param capacity The initial number of slots in the hashmap.
 * @param el_size The size of each value element in bytes.
 * @param grow_factor The factor by which the hashmap should grow when resized.
 * @return Pointer to the newly created hashmap_t, or NULL on allocation failure.
 */
inline hashmap_t* hashmap_new(const size_t capacity, const size_t el_size, const double grow_factor)
{
    hashmap_t* map = (hashmap_t*)malloc(sizeof(hashmap_t));
    if (!map) return NULL;

    map->entries = calloc(capacity, sizeof(hash_entry_t));
    if (!map->entries)
    {
        free(map);
        return NULL;
    }

    map->capacity = capacity;
    map->count = 0;
    map->el_size = el_size;
    map->grow_factor = grow_factor;
    return map;
}

/**
 * @brief Frees all memory associated with the given hashmap.
 *
 * This function releases the memory allocated for the hashmap structure,
 * including all keys stored in occupied entries and the entries array itself.
 * If the map pointer is NULL, the function does nothing.
 *
 * @param map Pointer to the hashmap to free.
 */
inline void hashmap_free(hashmap_t* map)
{
    if (!map) return;
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i].status == OCCUPIED)
        {
            free(map->entries[i].key);
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
 * @param hash_fn Function pointer for a custom hash function.
 * @return 1 on successful insertion, 0 on failure (e.g., allocation failure or invalid input).
 */
inline int hashmap_insert(hashmap_t* map, void* key, void *value, const hash_function_t hash_fn)
{
    if (!map || !key) return 0;
    if (map->count + 1 > map->capacity * 0.9)
    {
        if (!hashmap_resize(map, map->capacity * 2)) return 0;
    }

    const uint32_t hash = hash_fn(key);
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
        const size_t existing_dist = hash_probe_distance_t(map->entries[index].hash, index, map->capacity);
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
 */
inline int hashmap_resize(hashmap_t* map, const size_t new_capacity)
{
    hashmap_t* new_map = hashmap_new(new_capacity, map->el_size, map->grow_factor);
    if (!new_map) return 0;

    // Reinsert all existing entries
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i].status == OCCUPIED)
        {
            if (!hashmap_insert(new_map, map->entries[i].key, map->entries[i].value))
            {
                hashmap_free(new_map);
                return 0;
            }
        }
    }

    // Free old entries and update map
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i].status == OCCUPIED)
        {
            free(map->entries[i].key);
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
 * @param hash_fn Function pointer for a custom hash function.
 * @return 1 if the key was found and removed, 0 otherwise.
 */
inline int hashmap_remove(hashmap_t* map, void* key, const hash_function_t hash_fn)
{
    if (!map || !key) return 0;
    const uint32_t hash = hash_fn(key);
    size_t index = hash % map->capacity;

    while (map->entries[index].status != EMPTY)
    {
        if (
            map->entries[index].status == OCCUPIED &&
            map->entries[index].hash == hash &&
            strcmp(map->entries[index].key, key) == 0
        )
        {
            free(map->entries[index].key);
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
 * @brief Advances the iterator to the next occupied entry in the hashmap.
 *
 * This function iterates through the hashmap entries starting from the current index
 * of the iterator. It returns a pointer to the next OCCUPIED entry, or NULL if there
 * are no more occupied entries.
 *
 * @param iter Pointer to the hashmap iterator.
 * @return Pointer to the next occupied hash_entry_t, or NULL if iteration is complete.
 */
inline hash_entry_t* hashmap_iter_next(hashmap_iter_t* iter)
{
    if (!iter || !iter->map) return NULL; // Check for valid iterator and map

    // Iterate through the entries array
    while (iter->index < iter->map->capacity) {
        hash_entry_t* entry = &iter->map->entries[iter->index++];
        if (entry->status == OCCUPIED) {
            return entry; // Return pointer to the next occupied entry
        }
    }

    return NULL; // No more occupied entries
}
    
// ============= FLUENT LIB C++ =============
#if defined(__cplusplus)
}
#endif

#endif //HASHMAP_LIBRARY_H