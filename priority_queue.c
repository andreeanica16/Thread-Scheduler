#include <stdio.h>
#include <stdlib.h>

#include "priority_queue.h"

int init_priority_queue(priority_queue **pq, int number_of_levels)
{
	priority_queue *pr_queue;
	int i, ret;

	pr_queue = malloc(sizeof(priority_queue));
	if (pr_queue == NULL)
		return ERROR;

	pr_queue->number_of_levels = number_of_levels;
	pr_queue->levels = malloc(number_of_levels * sizeof(queue *));
	if (pr_queue->levels == NULL)
		return ERROR;

	for (i = 0; i < number_of_levels; i++) {
		ret = init_queue(&(pr_queue->levels[i]));
		if (ret == ERROR)
			return ERROR;
	}

	*pq = pr_queue;

	return SUCCESS;
}

void free_priority_queue(priority_queue *pq)
{
	int i;

	for (i = 0; i < pq->number_of_levels; i++)
		free_queue(pq->levels[i]);

	free(pq->levels);
	free(pq);
}

int is_priority_queue_empty(priority_queue *pq)
{
	int i;

	for (i = pq->number_of_levels - 1; i >= 0; i--) {
		if (!is_queue_empty(pq->levels[i]))
			return 0;
	}

	return 1;
}

tid_t head_priority_queue(priority_queue *pq)
{
	int i;

	for (i = pq->number_of_levels - 1; i >= 0; i--) {
		if (!is_queue_empty(pq->levels[i]))
			return head_queue(pq->levels[i]);
	}

	return -1;
}

int push_prioriry_queue(priority_queue *pq, tid_t value, int level)
{
	int ret;

	ret = push_queue(pq->levels[level], value);
	if (ret == ERROR)
		return ERROR;

	return SUCCESS;
}

void pop_prioriry_queue(priority_queue *pq)
{
	int i;

	for (i = pq->number_of_levels - 1; i >= 0; i--) {
		if (!is_queue_empty(pq->levels[i])) {
			pop_queue(pq->levels[i]);
			return;
		}
	}
}

void pop_from_level_prioriry_queue(priority_queue *pq, int level)
{
	pop_queue(pq->levels[level]);
}

void print_priority_queue(priority_queue *pq)
{
	int i;

	for (i = pq->number_of_levels - 1; i >= 0; i--) {
		printf("Level %d: ", i);
		print_queue(pq->levels[i]);
		printf("\n");
	}
}


