CC = gcc
CFLAGS = -Wall -lpthread -fPIC -g

.PHONY: build
build: libscheduler.so

libscheduler.so: hashmap.o  queue.o priority_queue.o so_scheduler.o
	$(CC) -shared -o $@ $^ -g

hashmap.o: hashmap.c hashmap.h so_scheduler.h
	$(CC) $(CFLAGS) -o $@ -c $<

queue.o: queue.c queue.h so_scheduler.h
	$(CC) $(CFLAGS) -o $@ -c $<

priority_queue.o: priority_queue.c priority_queue.h queue.h so_scheduler.h
	$(CC) $(CFLAGS) -o $@ -c $<

so_scheduler.o: so_scheduler.c so_scheduler.h so_scheduler_info.h so_thread_info.h hashmap.h priority_queue.h queue.h utils.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm hasmap.o queue.o priority_queue.o so_scheduler.o libscheduler.so
