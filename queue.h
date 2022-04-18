#ifndef QUEUE_H_
#define QUEUE_H_	1

#include "so_scheduler.h"

#ifndef ERROR
#define ERROR -1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef EMPTY
#define EMPTY -1
#endif

typedef struct node {
	tid_t value;
	struct node *next;
} node;

typedef struct queue {
	node *head, *tail;
} queue;

int init_queue(queue **q);
void free_queue(queue *q);
int is_queue_empty(queue *q);
tid_t head_queue(queue *q);
int push_queue(queue *q, tid_t value);
void pop_queue(queue *q);
void print_queue(queue *q);

#endif
