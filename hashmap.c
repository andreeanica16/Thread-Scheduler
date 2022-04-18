#include <stdlib.h>
#include <stdio.h>

#include "hashmap.h"

/*
 * Returns the current load factor of the map
 */
double load_factor(hashmap *map)
{
	if (map->maximum_size == 0)
		return 0;
	else
		return (double)map->size / map->maximum_size;
}

unsigned int hash_sum_calculator(tid_t key)
{
	return (unsigned int)key;
}

int replace_value(hashmap_entry *entry, void *new_value)
{
	entry->value = new_value;

	return 1;
}

/*
 * Returns the position in the hasmap where the elemennt
 * should be
 */
unsigned int index_calculator(int size, unsigned int hash_sum)
{
	return hash_sum % size;
}

int create_hasmap_entry(tid_t key, void *value, hashmap_entry **new_entry)
{
	hashmap_entry *entry;

	entry = malloc(sizeof(hashmap_entry));
	if (entry == NULL)
		return ERROR;

	entry->key = key;

	entry->value = value;

	entry->next = NULL;
	entry->hash_sum = hash_sum_calculator(key);

	*new_entry = entry;

	return SUCCESS;
}

int create_hashmap(hashmap **map)
{
	hashmap *aux;
	int i;

	aux = malloc(sizeof(hashmap));
	if (aux == NULL)
		return ERROR;

	aux->maximum_size = MINIMUM_HASHMAP_SIZE;
	aux->size = 0;
	aux->bucket = malloc(aux->maximum_size * sizeof(hashmap_entry *));
	if (aux->bucket == NULL)
		return ERROR;

	for (i = 0; i < aux->maximum_size; i++)
		aux->bucket[i] = NULL;

	*map = aux;
	return SUCCESS;
}

/*
 * If the load factor becomes grater than LOAD_LIMIT,
 * double the size of the hasmap
 */
int reallocate_hashmap(hashmap *map)
{
	hashmap_entry **new_bucket;
	hashmap_entry *iterator, *parent_iterator;
	int i, new_index;
	int new_size = map->maximum_size * 2;

	new_bucket = malloc(new_size * sizeof(hashmap_entry *));
	if (new_bucket == NULL)
		return ERROR;

	for (i = 0; i < new_size; i++)
		new_bucket[i] = NULL;

	for (i = 0; i < map->maximum_size; i++) {
		iterator = map->bucket[i];

		while (iterator != NULL) {
			parent_iterator = iterator;
			iterator = iterator->next;

			new_index = index_calculator(new_size, parent_iterator->hash_sum);
			parent_iterator->next = new_bucket[new_index];
			new_bucket[new_index] = parent_iterator;
		}
	}

	map->maximum_size = new_size;
	free(map->bucket);
	map->bucket = new_bucket;

	return SUCCESS;
}

int add_new_entry(hashmap *map, tid_t key, void *value)
{
	hashmap_entry *new_entry;
	int ret;
	unsigned int index;

	ret = create_hasmap_entry(key, value, &new_entry);
	if (ret == ERROR)
		return ERROR;

	index = index_calculator(map->maximum_size, new_entry->hash_sum);
	new_entry->next = map->bucket[index];
	map->bucket[index] = new_entry;

	map->size++;

	return SUCCESS;
}

hashmap_entry *find(hashmap *map, tid_t key)
{
	unsigned int hash, index;
	hashmap_entry *iterator;

	hash = hash_sum_calculator(key);
	index = index_calculator(map->maximum_size, hash);

	iterator = map->bucket[index];
	while (iterator != NULL && iterator->key - key != 0)
		iterator = iterator->next;

	return iterator;
}

int put_hashmap(hashmap *map, tid_t key, void *value)
{
	hashmap_entry *found_entry;
	double load;
	int ret;

	found_entry = find(map, key);
	if (found_entry != NULL)
		return replace_value(found_entry, value);

	load = load_factor(map);
	if (load > LOAD_LIMIT) {
		ret = reallocate_hashmap(map);
		if (ret == ERROR)
			return ERROR;
	}

	ret = add_new_entry(map, key, value);
	if (ret == ERROR)
		return ERROR;

	return SUCCESS;
}

void remove_entry_hashmap(hashmap *map, tid_t key, void (*free_value) (void *value))
{
	unsigned int hash, index;
	hashmap_entry *iterator, *parent;

	hash = hash_sum_calculator(key);
	index = index_calculator(map->maximum_size, hash);

	iterator = map->bucket[index];
	parent = map->bucket[index];
	while (iterator != NULL && iterator->key - key != 0) {
		parent = iterator;
		iterator = iterator->next;
	}

	if (iterator != NULL) {
		if (parent == iterator)
			map->bucket[index] = iterator->next;
		else
			parent->next = iterator->next;

		destroy_hashmap_entry(iterator, free_value);
	}
}

void *get_hashmap(hashmap *map, tid_t key)
{
	hashmap_entry *found_entry;

	found_entry = find(map, key);
	if (found_entry != NULL)
		return found_entry->value;
	else
		return NULL;
}

void destroy_hashmap_entry(hashmap_entry *entry, void (*free_value) (void *value))
{
	free_value(entry->value);
	free(entry);
}

void destroy_hashmap(hashmap *map, void (*free_value) (void *value))
{
	int i;
	hashmap_entry *iterator;
	hashmap_entry *parent;

	if (map == NULL)
		return;

	for (i = 0; i < map->maximum_size; i++) {
		iterator = map->bucket[i];

		while (iterator != NULL) {
			parent = iterator;
			iterator = iterator->next;
			destroy_hashmap_entry(parent, free_value);
		}
	}

	free(map->bucket);
	free(map);
}

void print_hashmap(hashmap *map)
{
	int i;

	for (i = 0; i < map->maximum_size; i++) {
		hashmap_entry *iterator = map->bucket[i];

		while (iterator != NULL) {
			printf("%ld --> %p\n", iterator->key, iterator->value);
			iterator = iterator->next;
		}
	}
}
