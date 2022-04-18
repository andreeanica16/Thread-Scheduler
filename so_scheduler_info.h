#ifndef SOSCHEDULERINFO_H_
#define SOSCHEDULERINFO_H_	1

#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "hashmap.h"
#include "priority_queue.h"

#ifndef ERROR
#define ERROR -1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#define LOOP_COUNT 100

typedef struct so_scheduler {
	hashmap *info_threads;
	priority_queue *pq;

	queue **wait_list;
	unsigned int time_quantum, io;
	pthread_mutex_t mutex_pq;
	sem_t signal_end;
	int current_quantum;
	int started;
} so_scheduler;

so_scheduler *scheduler;

#endif
