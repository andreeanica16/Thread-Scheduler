#ifndef HASHMAP_H_
#define HASHMAP_H_	1

#include "so_scheduler.h"

#define MINIMUM_HASHMAP_SIZE 17
#define LOAD_LIMIT 0.7

#ifndef ERROR
#define ERROR -1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif


typedef struct hashmap_entry {
	tid_t key;
	void *value;
	unsigned int hash_sum;
	struct hashmap_entry *next;
} hashmap_entry;

typedef struct hashmap {
	int size;
	int maximum_size;
	struct hashmap_entry **bucket;
} hashmap;

int create_hasmap_entry(tid_t key, void *value, hashmap_entry **new_entry);
int create_hashmap(hashmap **map);
int reallocate_hashmap(hashmap *map);
int add_new_entry(hashmap *map, tid_t key, void *value);
hashmap_entry *find(hashmap *map, tid_t key);
int put_hashmap(hashmap *map, tid_t key, void *value);
void *get_hashmap(hashmap *map, tid_t key);
void remove_entry_hashmap(hashmap *map, tid_t key, void (*free_value) (void *value));
void destroy_hashmap(hashmap *map, void (*free_value) (void *value));
void destroy_hashmap_entry(hashmap_entry *entry, void (*free_value) (void *value));
void print_hashmap(hashmap *map);

#endif
