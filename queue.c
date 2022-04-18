#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

int init_queue(queue **q)
{
	queue *new_queue;

	new_queue = malloc(sizeof(queue));
	if (new_queue == NULL)
		return ERROR;

	new_queue->head = NULL;
	new_queue->tail = NULL;

	*q = new_queue;

	return SUCCESS;
}

node *init_node (tid_t value)
{
	node *new_node;

	new_node = malloc(sizeof(node));
	if (new_node == NULL)
		return NULL;

	new_node->value = value;
	new_node->next = NULL;

	return new_node;
}

int is_queue_empty(queue *q)
{
	return (q->head == NULL);
}

int push_queue(queue *q, tid_t value)
{
	node *new_node;

	new_node = init_node(value);
	if (new_node == NULL)
		return ERROR;

	if (is_queue_empty(q)) {
		q->head = new_node;
		q->tail = new_node;
	} else {
		q->tail->next = new_node;
		q->tail = new_node;
	}

	return SUCCESS;
}

tid_t head_queue(queue *q)
{
	if (q->head != NULL)
		return q->head->value;
	else
		return EMPTY;
}

void pop_queue(queue *q)
{
	node *aux;

	if (q->head == q->tail) {
		if (q->head != NULL)
			free(q->head);

		q->head = NULL;
		q->tail = NULL;
	} else {
		aux = q->head;
		q->head = q->head->next;

		free(aux);
	}
}

void free_queue(queue *q)
{
	node *iterator, *aux;

	iterator = q->head;
	while (iterator != NULL) {
		aux = iterator;
		iterator = iterator->next;

		free(aux);
	}

	free(q);
}

void print_queue(queue *q)
{
	node *iterator;

	if (is_queue_empty(q)) {
		printf("Queue is empty\n");
		return;
	}

	printf(
		"head %ld tail %ld, elements : ",
		q->head->value,
		q->tail->value
	);

	iterator = q->head;
	while (iterator != NULL) {
		printf("%ld ", iterator->value);
		iterator = iterator->next;
	}

	printf("\n");
}
