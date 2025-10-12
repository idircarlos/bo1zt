#include "map.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ZCOUNT_OF(arr) (sizeof(arr) / sizeof(*arr))

struct MapEntry {
  char *key;
  void *val;
  struct MapEntry *next;
};

struct Map {
  size_t size_index;
  size_t entry_count;
  struct MapEntry **entries;
};

static struct MapEntry *_mapEntryCreate(const char *key, void *val);
static void _mapEntryDestroy(struct MapEntry *entry, bool recursive);
static size_t _mapGenerateHash(struct Map *map, const char *key);
static void _mapRehash(struct Map *map, size_t size_index);
static size_t _mapNextSizeIndex(size_t size_index);
static size_t _mapPreviousSizeIndex(size_t size_index);
static struct Map *_MapCreateWithSize(size_t size_index);
static void *_mapAuxMalloc(size_t size);
static void *_mapAuxCalloc(size_t num, size_t size);

// possible sizes for hash table; must be prime numbers
static const size_t hash_sizes[] = {
    53, 101, 211, 503, 1553, 3407, 6803, 12503, 25013, 50261,
    104729, 250007, 500009, 1000003, 2000029, 4000037, 10000019,
    25000009, 50000047, 104395301, 217645177, 512927357, 1000000007
};

// functions declared in map.h
struct Map *mapCreate(void) {
    return _MapCreateWithSize(0);
}

void mapDestroy(struct Map *map) {
    size_t size, ii;

    size = hash_sizes[map->size_index];

    for (ii = 0; ii < size; ii++) {
        struct MapEntry *entry;
        if ((entry = map->entries[ii])) {
            _mapEntryDestroy(entry, true);
        }
    }
    free((void *) map->entries);
    free((void *) map);
}

void mapPut(struct Map *map, const char *key, void *val) {
    size_t size, hash;
    struct MapEntry *entry;

    hash = _mapGenerateHash(map, key);
    entry = map->entries[hash];

    while (entry) {
        if (strcmp(key, entry->key) == 0) {
            entry->val = val;
            return;
        }
        entry = entry->next;
    }

    entry = _mapEntryCreate(key, val);
    entry->next = map->entries[hash];
    map->entries[hash] = entry;
    map->entry_count++;

    size = hash_sizes[map->size_index];

    if (map->entry_count > size / 2) {
        _mapRehash(map, _mapNextSizeIndex(map->size_index));
    }
}

void *mapGet(struct Map *map, const char *key) {
    size_t hash;
    struct MapEntry *entry;

    hash = _mapGenerateHash(map, key);
    entry = map->entries[hash];

    while (entry && strcmp(key, entry->key) != 0) {
        entry = entry->next;
    }
    return entry ? entry->val : NULL;
}

void *mapRemove(struct Map *map, const char *key) {
    size_t size, hash;
    struct MapEntry *entry;
    void *val;

    hash = _mapGenerateHash(map, key);
    entry = map->entries[hash];

    if (entry && strcmp(key, entry->key) == 0) {
        map->entries[hash] = entry->next;
    } 
    else {
        while (entry) {
            if (entry->next && strcmp(key, entry->next->key) == 0) {
                struct MapEntry *deleted_entry;
                deleted_entry = entry->next;
                entry->next = entry->next->next;
                entry = deleted_entry;
                break;
            }
            entry = entry->next;
        }
    }

    if (!entry) return NULL;

    val = entry->val;
    _mapEntryDestroy(entry, false);
    map->entry_count--;

    size = hash_sizes[map->size_index];

    if (map->entry_count < size / 8) {
        _mapRehash(map, _mapPreviousSizeIndex(map->size_index));
    }

    return val;
}

bool mapContains(struct Map *map, const char *key) {
    size_t hash;
    struct MapEntry *entry;

    hash = _mapGenerateHash(map, key);
    entry = map->entries[hash];

    while (entry && strcmp(key, entry->key) != 0) {
        entry = entry->next;
    }
    return entry ? true : false;
}

// helper functions, definitions
static struct Map *_MapCreateWithSize(size_t size_index) {
    struct Map *map;

    map = (struct Map *) _mapAuxMalloc(sizeof(struct Map));

    map->size_index = size_index;
    map->entry_count = 0;
    map->entries = (MapEntry**)_mapAuxCalloc(hash_sizes[size_index], sizeof(MapEntry*));

    return map;
}

static struct MapEntry *_mapEntryCreate(const char *key, void *val) {
    struct MapEntry *entry;
    char *key_cpy;

    key_cpy = (char *) _mapAuxMalloc((strlen(key) + 1) * sizeof(char));
    entry = (struct MapEntry *) _mapAuxMalloc(sizeof(struct MapEntry));

    strcpy(key_cpy, key);
    entry->key = key_cpy;
    entry->val = val;

    return entry;
}

static void _mapEntryDestroy(struct MapEntry *entry, bool recursive) {
    struct MapEntry *next;

    if (!recursive) {
        entry->next = NULL;
    }

    while (entry) {
        next = entry->next;

        free((void *) entry->key);
        free((void *) entry);

        entry = next;
    }
}

static size_t _mapGenerateHash(struct Map *map, const char *key) {
    size_t size, hash;
    char ch;

    size = hash_sizes[map->size_index];
    hash = 0;

    while ((ch = *key++)) {
        hash = (17 * hash + ch) % size;
    }
    return hash;
}

static void _mapRehash(struct Map *map, size_t size_index) {
    size_t hash, size, ii;
    struct MapEntry **entries;

    if (size_index == map->size_index) {
        return;
    }

    size = hash_sizes[map->size_index];
    entries = map->entries;

    map->size_index = size_index;
    map->entries = (MapEntry**)_mapAuxCalloc(hash_sizes[size_index], sizeof(MapEntry*));

    for (ii = 0; ii < size; ii++) {
        struct MapEntry *entry;

        entry = entries[ii];
        while (entry) {
            struct MapEntry *next_entry;
            hash = _mapGenerateHash(map, entry->key);
            next_entry = entry->next;
            entry->next = map->entries[hash];
            map->entries[hash] = entry;
            entry = next_entry;
        }
    }
    free((void *) entries);
}

static size_t _mapNextSizeIndex(size_t size_index) {
    if (size_index == ZCOUNT_OF(hash_sizes)) {
        return size_index;
    }
    return size_index + 1;
}

static size_t _mapPreviousSizeIndex(size_t size_index) {
    if (size_index == 0) {
        return size_index;
    }
    return size_index - 1;
}

static void *_mapAuxMalloc(size_t size) {
    void *ptr;
    ptr = malloc(size);
    if (!ptr) {
        printf("Coudln't allocate space for map\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void *_mapAuxCalloc(size_t num, size_t size) {
    void *ptr;
    ptr = calloc(num, size);
    if (!ptr) {
        printf("Coudln't allocate space for map\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}