#ifndef SOTHREADINFO_H_
#define SOTHREADINFO_H_	1

#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

typedef struct so_thread_info {
	so_handler *handler;
	unsigned int priority;
	int state;
	pthread_cond_t check_state;
	pthread_mutex_t mutex_state;
	sem_t signal_thread;
} so_thread_info;

#endif
