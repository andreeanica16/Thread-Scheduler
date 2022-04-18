# Thread Scheduler

## Introduction
Preemptive Thread Scheduler in C for POSIX threads which uses a Round Robin with priority algorithm.

## Description

The implementation is based on the so_scheduler structure, which contains
various elements so as to ensure that the planning of threads
is successfully performed.

A first element is the info_threads hashmap, which contains information about
each thread, such as status, handler, priority, and
more importantly, a traffic light used for thread synchronization. lights
signal_thread will lock the thread to which it belongs until it is
planned on the processor. It also contains synchronization elements, such as
a mutex or a condition variable, required for signaling to
the parent thread (the one that called the thread with the fork command) that the startup of the
thread is ready.

In order to determine the correct order of threads by priority I used a priority_queue. For
efficient insertion of elements in the priorty queue, without sorting the elements each time, it consists a
separate queue for each priority level. When
we want to add a thread in the queue, we will add it in the corresponding queue based on 
its priority.
The removal of the first element will be done from the head of the highest priority
queue.

For so_wait and so_signal operations it was necessary to enter a
new element in the so_scheduler structure, namely a wait_list vector,
each element of the vector being the waiting list associated with an event. Thus, when we call so_wait, the thread enters the
corresponding waiting list, being awakened by so_signal's call, once
with all other threads in the list. 

The field current_quantum represents the time
remaining to the current thread until it is preempted.

In so_scheduler we also find some necessary synchronization elements:

## How planning works?

1. At the beginning of an operation (such as so_fork, so_exec, so_signal)
we will decrease the remaining time of the current thread on the processor. If this
is the last operation of the thread to be performed until it enters preemption,
then we will remove the thread from the top of the queue with priority. This is done by the mark_quantum function.
2. The function will execute the operation for which it was called
3. If the thread has been removed from the priority queue because it
was preempted, then we'll have to add it back in the queue in order to be planned later. This removal from the queuq (when
CPU time of current thread has expired)
and adding back after performing the operation offers several advantages,
making it easier to find the next thread. An example would be if
thread 0 starts a thread with the same priority as his, say
thread 1, thread 1 will be added to the queue, and thread 0
will be re-queued after thread 1, so thread 1 will be
planned, avoiding the situation in which thread 0 is planned again.
4. At the end, we call the schedule_next_thread function, which determines
which is the next thread to run. To correctly determine
which is the next thread, it investigates the head of the priority queue.
The priority queue changes when new threads are added
for planning, for example after a fork or signal operation.
However, the head of the queue will always tell us who is
the highest priority thread to be planned.
The current thread is set to the following appropriate state (WAITING, READY,
TERMINATED), and the new thread that will run will be woken up with the help
the traffic light associated with it. Depending on the following state, the thread will
be put on hold (WAITING, READY), or will mark the end of its execution
(TERMINATED). If the current thread is scheduled again, then the only change made will be the reallocation of an entire amount of time.
5. The only exception to the algorithm described above is the so_wait function, in which
the thread is preempted no matter how long it is, and it shouldn't
re-queues with priority than the corresponding so_signal call.
Therefore, when calling so_wait, the thread will be removed from the priority queue
regardless, and will be added to the device's queue, blocking it
until it is signaled and planned.

## How to compile the code?
In a terminal, run:
```
	make build
```



