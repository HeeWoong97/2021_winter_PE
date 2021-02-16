#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define BILLION	1000000000
#define NUM_OF_DATA	100000

int total_size = 0, size_per_thread = 0;
int numbers[1000001];
tree_node *root;

int finish;

bool remove_dbg = false; // dbg_printf
extern pthread_mutex_t show_tree_lock;

/* function headers */
void generate_data(void);
int run_multi_thread_insert(int thread_count, int num_of_data);
int run_multi_thread_remove(int thread_count, int num_of_data);
void run_serial();
void run_insert_remove();
unsigned long long calclock(struct timespec *spclock, unsigned long long *total_time, unsigned long long *total_count);

int main(int argc, char **argv)
{
	int threads_num[3] = {1, 2, 4};
	int data[4] = {25000, 50000, 75000, 100000};

    // test settings
    int num_processes_i = 1;
    int num_processes_r = 1;
	int thread_num;

    generate_data();
	for (int j = 0; j < 4; j++) {
		printf("total_size: %d\n", data[j]);
		for (int i = 0; i < 3; i++) {
			thread_num = threads_num[i];
			num_processes_r = num_processes_i = thread_num;

			root = rb_init();

			run_multi_thread_insert(num_processes_i, data[j]);
			run_multi_thread_remove(num_processes_r, data[j]);
		}
		printf("\n");
	}
	printf("\n");
    return 0;
}

void *run_insert(void *i)
{
    int *p = numbers + ((long)i) * size_per_thread;
    thread_index_init((long) i);
    int *start = p;
    int count = size_per_thread;
    for (int i = 0; i < count; i++) {
        int element = start[i];
        rb_insert(root, element);
        dbg_printf("[RUN] finish inserting element %d\n", element);
    }
	__sync_fetch_and_add(&finish, 1);

    return NULL;
}

int run_multi_thread_insert(int thread_count, int num_of_data)
{
    pthread_t tid[thread_count];
    size_per_thread = num_of_data / thread_count;

	struct timespec spclock[2];
	unsigned long long time = 0;
	unsigned long long count = 0;

	clock_gettime(CLOCK_REALTIME, &spclock[0]);

	finish = -thread_count;
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&tid[i], NULL, run_insert, (void *)(i));
    }

	/*
    for (int i = 0; i < thread_count; i++) {
        pthread_join(tid[i], NULL);
    }

    clock_gettime(CLOCK_REALTIME, &spclock[1]);
	calclock(spclock, &time, &count);
    */

	while (__sync_fetch_and_add(&finish, 0)) {
		// usleep(100);
	}

	if (!__sync_fetch_and_add(&finish, 0)) {
		clock_gettime(CLOCK_REALTIME, &spclock[1]);
		calclock(spclock, &time, &count);
	}

	for (int i = 0; i < thread_count; i++) {
		pthread_cancel(tid[i]);
	}

	printf("time taken by insert with %d thread: %llu nsec\n", thread_count, time);

    return 0;
}

void *run_remove(void *i)
{
    int *p = numbers + ((long)i) * size_per_thread;
    thread_index_init((long)i);
    int *start = p;
    int count = size_per_thread;
    for (int j = 0; j < count; j++) {
        int element = start[j];
        rb_remove(root, element);
        dbg_printf("[RUN] finish removing element %d\n", element);
    }
	__sync_fetch_and_add(&finish, 1);

    return NULL;
}

int run_multi_thread_remove(int thread_count, int num_of_data)
{
    pthread_t tid[thread_count];
    size_per_thread = num_of_data / thread_count;

	struct timespec spclock[2];
	unsigned long long time = 0;
	unsigned long long count = 0;

	clock_gettime(CLOCK_REALTIME, &spclock[0]);

	finish = -thread_count;
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&tid[i], NULL, run_remove, (void *)(i));
    }

	/*
    for (int i = 0; i < thread_count; i++) {
        pthread_join(tid[i], NULL);
    }

	clock_gettime(CLOCK_REALTIME, &spclock[1]);
	calclock(spclock, &time, &count);
    */

	while (__sync_fetch_and_add(&finish, 0)) {
		// usleep(100);
	}

	// if (!__sync_fetch_and_add(&finish, 0)) {
		clock_gettime(CLOCK_REALTIME, &spclock[1]);
		calclock(spclock, &time, &count);
	// }

	for (int i = 0; i < thread_count; i++) {
		pthread_cancel(tid[i]);
	}

	printf("time taken by remove with %d thread: %llu nsec\n", thread_count,  time);

    return 0;
}

void generate_data(void)
{
	int temp;
	int random;

	srand(time(NULL));
	for (int i = 0; i < NUM_OF_DATA; i++) {
		numbers[i] = i + 1;
		total_size++;
	}
	for (int i = 0; i < NUM_OF_DATA - 1; i++) {
		random = rand() % (NUM_OF_DATA - i) + i;
		temp = numbers[i];
		numbers[i] = numbers[random];
		numbers[random] = temp;
	}
}	

unsigned long long calclock(struct timespec *spclock, unsigned long long *total_time, unsigned long long *total_count)
{
    long temp, temp_n;
    unsigned long long timedelay = 0;
    if (spclock[1].tv_nsec >= spclock[0].tv_nsec) {
        temp = spclock[1].tv_sec - spclock[0].tv_sec;
        temp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }
    else {
        temp = spclock[1].tv_sec - spclock[0].tv_sec - 1;
        temp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }

    __sync_fetch_and_add(total_time, timedelay);
    __sync_fetch_and_add(total_count, 1);

    return timedelay;
}

