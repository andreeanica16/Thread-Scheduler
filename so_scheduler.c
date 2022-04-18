#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "so_scheduler.h"
#include "so_scheduler_info.h"
#include "so_thread_info.h"
#include "hashmap.h"
#include "priority_queue.h"
#include "queue.h"
#include "utils.h"

int so_init(unsigned int time_quantum, unsigned int io)
{
	int ret, i;

	/*
	 * Check for errors
	 */
	if (scheduler != NULL)
		return ERROR;

	if (io > SO_MAX_NUM_EVENTS)
		return ERROR;

	if (time_quantum <= 0)
		return ERROR;

	scheduler = malloc(sizeof(so_scheduler));
	if (scheduler == NULL)
		return ERROR;

	ret = create_hashmap(&(scheduler->info_threads));
	if (ret == ERROR)
		return ERROR;

	ret = init_priority_queue(&(scheduler->pq), SO_MAX_PRIO + 1);
	if (ret == ERROR)
		return ERROR;

	scheduler->wait_list = malloc((io + 1) * sizeof(queue *));
	if (scheduler->wait_list == NULL)
		return ERROR;

	for (i = 0; i <= io; i++) {
		ret = init_queue(&scheduler->wait_list[i]);
		if (ret == ERROR)
			return ERROR;
	}

	ret = pthread_mutex_init(&(scheduler->mutex_pq), NULL);
	if (ret != 0)
		return ERROR;

	ret = sem_init(&(scheduler->signal_end), 0, 0);
	if (ret != 0)
		return ERROR;

	scheduler->io = io;
	scheduler->time_quantum = time_quantum;
	scheduler->current_quantum = time_quantum;
	scheduler->started = 0;

	return SUCCESS;
}

/*
 * Add thread to the scheduling queue
 */
int register_thread(tid_t tid, unsigned int priority)
{
	int ret;

	ret = pthread_mutex_lock(&scheduler->mutex_pq);
	if (ret != 0)
		return ERROR;

	ret = push_prioriry_queue(scheduler->pq, tid, priority);
	if (ret == ERROR)
		return ERROR;

	ret = pthread_mutex_unlock(&scheduler->mutex_pq);
	if (ret != 0)
		return ERROR;

	return SUCCESS;
}

/*
 * Remove thread from scheduling queue
 */
int unregister_thread(void)
{
	int ret;

	ret = pthread_mutex_lock(&scheduler->mutex_pq);
	if (ret != 0)
		return ERROR;

	pop_prioriry_queue(scheduler->pq);

	ret = pthread_mutex_unlock(&scheduler->mutex_pq);
	if (ret != 0)
		return ERROR;

	return SUCCESS;
}

/*
 * Decresea the time-quantum of a thread with a new instruction
 * that it executes. If the allocated time-quantum expired,
 * remove the thread from the scheduling queue
 */
int mark_quantum(int *is_not_in_queue)
{
	int ret;

	if (!scheduler->started)
		return SUCCESS;

	scheduler->current_quantum--;
	/*
	 * If the quantum expired, remove from scheduling queue
	 */
	if (scheduler->current_quantum == 0) {
		*is_not_in_queue = 1;
		ret = unregister_thread();
		if (ret == ERROR)
			return ERROR;
	} else
		*is_not_in_queue = 0;

	return SUCCESS;
}

/*
 * Determine next thread to be planned, start it and put the current thread
 * into the corresponding state (WAITING, TERMINATED, READY)
 */
int schedule_next_thread(tid_t tid, so_thread_info *thread_info,
	int is_not_in_queue, int next_state)
{
	int ret;
	tid_t next_tid;
	so_thread_info *next_thread_info;

	ret = pthread_mutex_lock(&scheduler->mutex_pq);
	if (ret != 0)
		return ERROR;

	next_tid = head_priority_queue(scheduler->pq);

	ret = pthread_mutex_unlock(&scheduler->mutex_pq);
	if (ret != 0)
		return ERROR;

	if (next_tid == EMPTY) {
		/*
		 * There are no instructions left. Scheduler job is done
		 */
		ret = sem_post(&scheduler->signal_end);
		if (ret != 0)
			return ERROR;
	} else if (next_tid == tid) {
		/*
		 * If current thread's time-quantum expired and it's rescheduled
		 * again, reallocate the time_quantum
		 */
		if (is_not_in_queue)
			scheduler->current_quantum = scheduler->time_quantum;
	} else {
		next_thread_info = (so_thread_info *) get_hashmap(
			scheduler->info_threads,
			next_tid
		);

		next_thread_info->state = RUNNING;

		if (thread_info != NULL)
			thread_info->state = next_state;

		scheduler->current_quantum = scheduler->time_quantum;

		ret = sem_post(&next_thread_info->signal_thread);
		if (ret != 0)
			return ERROR;

		if (thread_info != NULL && thread_info->state != TERMINATED) {
			ret = sem_wait(&thread_info->signal_thread);
			if (ret != 0)
				return ERROR;
		}
	}

	return SUCCESS;
}

/*
 * Helper function called by pthread_create
 */
void *start_thread(void *params)
{
	so_thread_info *thread_info = *(so_thread_info **) params;
	tid_t tid;
	int ret;

	tid = pthread_self();

	ret = put_hashmap(scheduler->info_threads, tid, thread_info);
	DIE(ret == ERROR, "put_hashmap");

	ret = register_thread(tid, thread_info->priority);
	DIE(ret == ERROR, "register_thread");

	/*
	 * Mark the thread as READY
	 */
	ret = pthread_mutex_lock(&thread_info->mutex_state);
	DIE(ret != 0, "pthread_mutex_lock");

	thread_info->state = READY;

	ret = pthread_cond_signal(&thread_info->check_state);
	DIE(ret != 0, "pthread_cond_signal");

	ret = pthread_mutex_unlock(&thread_info->mutex_state);
	DIE(ret != 0, "pthread_mutex_unlock");

	/*
	 * Wait the thread to be scehduled
	 */
	ret = sem_wait(&thread_info->signal_thread);
	DIE(ret != 0, "semaphore wait");

	/*
	 * Run the thread
	 */
	thread_info->handler(thread_info->priority);

	/*
	 * The thread terminated its action. Remove it from the scheduler
	 */
	ret = unregister_thread();
	DIE(ret == ERROR, "unregister_thread");

	ret = schedule_next_thread(tid, thread_info, 1, TERMINATED);
	DIE(ret == ERROR, "schedule_next_thread");

	pthread_exit(NULL);
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	so_thread_info *thread_info, *current_thread_info;
	tid_t tid, current_tid;
	int ret, is_not_in_queue;

	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;

	/*
	 * Consume one time quantum for this action
	 */
	ret = mark_quantum(&is_not_in_queue);
	if (ret == ERROR)
		return INVALID_TID;

	scheduler->started = 1;

	/*
	 * Initialise thread struct
	 */
	thread_info = malloc(sizeof(so_thread_info));
	if (thread_info == NULL)
		return INVALID_TID;

	thread_info->handler = func;
	thread_info->priority = priority;
	thread_info->state = NEW;
	ret = pthread_cond_init(&(thread_info->check_state), NULL);
	if (ret != 0)
		return INVALID_TID;

	ret = pthread_mutex_init(&(thread_info->mutex_state), NULL);
	if (ret != 0)
		return INVALID_TID;

	ret = sem_init(&(thread_info->signal_thread), 0, 0);
	if (ret != 0)
		return INVALID_TID;

	ret = pthread_create(&tid, NULL, start_thread, &thread_info);
	if (ret != 0)
		return INVALID_TID;

	ret = pthread_mutex_lock(&thread_info->mutex_state);
	if (ret != 0)
		return INVALID_TID;

	/*
	 * Wait until the thread enters READY/RUN state
	 */
	while (thread_info->state != RUNNING &&
		thread_info->state != READY) {
		ret = pthread_cond_wait(
			&thread_info->check_state,
			&thread_info->mutex_state
		);
		if (ret != 0)
			return INVALID_TID;
	}

	ret = pthread_mutex_unlock(&thread_info->mutex_state);
	if (ret != 0)
		return INVALID_TID;

	current_tid = pthread_self();
	current_thread_info = (so_thread_info *) get_hashmap(
		scheduler->info_threads,
		current_tid
	);

	/*
	 * If the thread is not in the scheduling queue, add it
	 */
	if (current_thread_info != NULL && is_not_in_queue) {
		register_thread(current_tid, current_thread_info->priority);
		if (ret == ERROR)
			return INVALID_TID;
	}

	ret = schedule_next_thread(
		current_tid,
		current_thread_info,
		is_not_in_queue,
		READY
	);
	if (ret == ERROR)
		return INVALID_TID;

	return tid;
}

int so_wait(unsigned int io)
{
	tid_t tid;
	so_thread_info *thread_info;
	int ret;

	if (io >= scheduler->io)
		return ERROR;

	tid = pthread_self();
	thread_info = (so_thread_info *) get_hashmap(
		scheduler->info_threads,
		tid
	);

	/*
	 * Add thread to the wainting list of the io event
	 */
	push_queue(scheduler->wait_list[io], tid);

	/*
	 * Remove it from the scheduler
	 */
	ret = unregister_thread();
	if (ret == ERROR)
		return ERROR;

	ret = schedule_next_thread(tid, thread_info, 1, WAITING);
	if (ret == ERROR)
		return ERROR;

	return SUCCESS;
}

int so_signal(unsigned int io)
{
	tid_t tid, awake_tid;
	so_thread_info *thread_info, *awake_thread_info;
	int ret, is_not_in_queue, count = 0;

	if (io >= scheduler->io)
		return ERROR;

	tid = pthread_self();
	thread_info = (so_thread_info *) get_hashmap(
		scheduler->info_threads,
		tid
	);

	ret = mark_quantum(&is_not_in_queue);
	if (ret == ERROR)
		return ERROR;

	/*
	 * Awake every thread wainting for the io event and add them
	 * to the scheduler, in order to properly plan the next thread
	 */
	while (!is_queue_empty(scheduler->wait_list[io])) {
		awake_tid = head_queue(scheduler->wait_list[io]);
		awake_thread_info = (so_thread_info *) get_hashmap(
			scheduler->info_threads,
			awake_tid
		);

		awake_thread_info->state = READY;
		ret = register_thread(awake_tid, awake_thread_info->priority);
		if (ret == ERROR)
			return ERROR;

		count++;
		pop_queue(scheduler->wait_list[io]);
	}

	if (is_not_in_queue) {
		register_thread(tid, thread_info->priority);
		if (ret == ERROR)
			return ERROR;
	}

	ret = schedule_next_thread(tid, thread_info, is_not_in_queue, READY);
	if (ret == ERROR)
		return ERROR;

	return count;
}

void so_exec(void)
{
	tid_t tid;
	so_thread_info *thread_info;
	int i, dummy = 0, ret, is_not_in_queue;

	tid = pthread_self();
	thread_info = (so_thread_info *) get_hashmap(
		scheduler->info_threads,
		tid
	);

	ret = mark_quantum(&is_not_in_queue);
	DIE(ret == ERROR, "mark_quantum");

	/*
	 * Executes a dummy operation
	 */
	for (i = 0; i < LOOP_COUNT; i++)
		dummy++;

	if (is_not_in_queue) {
		ret = register_thread(tid, thread_info->priority);
		DIE(ret == ERROR, "register_thread");
	}

	ret = schedule_next_thread(tid, thread_info, is_not_in_queue, READY);
	DIE(ret == ERROR, "schedule_next_thread");
}

/*
 * Helper function to properly free a thread_info struct.
 * Used to free de hasma info_threads
 */
void free_thread_info(void *info)
{
	so_thread_info *thread_info = (so_thread_info *) info;

	pthread_mutex_destroy(&thread_info->mutex_state);
	pthread_cond_destroy(&thread_info->check_state);
	sem_destroy(&thread_info->signal_thread);

	free(info);
}

/*
 * Wait for all threads to finish their execution
 */
void wait_threads_to_finish(void)
{
	int i, ret;
	hashmap_entry *iterator;
	hashmap *map;

	ret = sem_wait(&scheduler->signal_end);
	DIE(ret != 0, "sem_wait");

	map = scheduler->info_threads;

	for (i = 0; i < map->maximum_size; i++) {
		iterator = map->bucket[i];

		while (iterator != NULL) {
			pthread_join(iterator->key, NULL);
			iterator = iterator->next;
		}
	}
}

void so_end(void)
{
	int i;

	if (scheduler == NULL)
		return;

	if (scheduler->started)
		wait_threads_to_finish();

	destroy_hashmap(scheduler->info_threads, free_thread_info);

	free_priority_queue(scheduler->pq);

	for (i = 0; i <= scheduler->io; i++)
		free_queue(scheduler->wait_list[i]);
	free(scheduler->wait_list);

	sem_destroy(&scheduler->signal_end);

	pthread_mutex_destroy(&scheduler->mutex_pq);
	free(scheduler);

	scheduler = NULL;
}
