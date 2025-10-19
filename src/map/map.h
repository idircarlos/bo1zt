#ifndef MAP_H_
#define MAP_H_

#include <stddef.h>
#include <stdbool.h>


typedef struct MapEntry MapEntry;
typedef struct Map Map;

struct Map *mapCreate(void);
void mapDestroy(struct Map *hash_table);
void mapPut(struct Map *hash_table, const char *key, void *val);
void *mapGet(struct Map *hash_table, const char *key);
void *mapRemove(struct Map *hash_table, const char *key);
bool mapContains(struct Map *hash_table, const char *key);

#endif // MAP_H_