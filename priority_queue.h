#ifndef PRIORITY_H_
#define PRIORITY_H_	1

#include "so_scheduler.h"
#include "queue.h"

#ifndef ERROR
#define ERROR -1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef EMPTY
#define EMPTY -1
#endif

typedef struct priority_queue {
	int number_of_levels;

	queue **levels;
} priority_queue;

int init_priority_queue(priority_queue **pq, int number_of_levels);
void free_priority_queue(priority_queue *pq);
tid_t head_priority_queue(priority_queue *pq);
int push_prioriry_queue(priority_queue *pq, tid_t value, int level);
void pop_prioriry_queue(priority_queue *pq);
void pop_from_level_prioriry_queue(priority_queue *pq, int level);
void print_priority_queue(priority_queue *pq);

#endif
