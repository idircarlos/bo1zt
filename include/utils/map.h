#ifndef MAP_H_
#define MAP_H_

#include <stddef.h>
#include <stdbool.h>

#define mapPutInt(map, key, val) mapPut(map, key, (void*)(intptr_t)(val))
#define mapGetInt(map, key) ((int)(intptr_t)mapGet(map, key))

#define mapPutBool(map, key, val) mapPut(map, key, (void*)(intptr_t)(val))
#define mapGetBool(map, key) ((bool)(intptr_t)mapGet(map, key))


typedef struct Map Map;
typedef void (*MapIteratorFn)(const char *key, void *value, void *userData);

Map *mapCreate(void);
void mapDestroy(struct Map *hash_table);
void mapPut(struct Map *hash_table, const char *key, void *val);
void *mapGet(struct Map *hash_table, const char *key);
void *mapRemove(struct Map *hash_table, const char *key);
bool mapContains(struct Map *hash_table, const char *key);
void mapForEach(struct Map *map, MapIteratorFn fn, void *userData);

#endif // MAP_H_
