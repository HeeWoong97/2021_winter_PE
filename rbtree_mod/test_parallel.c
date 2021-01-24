#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
// #include "vector.h"
#include "vector_arr.h"

// #define COMPUTATION_TIME_SEC 0.3  // in sec
// #define COMPUTATION_TIME_USEC COMPUTATION_TIME_SEC * 1000000    // in usec

vector THREADS_NUM_LIST;

vector COMPUTATION_TIME_LIST;

vector test_time_list;

int total_size = 0, size_per_thread = 0;
int numbers[1000001];
tree_node *root;
int sleep_time = 0;

bool remove_dbg = false; // dbg_printf
extern pthread_mutex_t show_tree_lock;

/* function headers */
void load_data_from_txt();
int run_multi_thread_insert(int thread_count);
int run_multi_thread_remove(int thread_count);
void *run(void *p);
void run_serial();
void run_insert_remove();

int main(int argc, char **argv)
{
    // vector init
    int threads_num1 = 1, threads_num2 = 2, threads_num3 = 4, threads_num4 = 8, threads_num5 = 16;
    vector_init(&THREADS_NUM_LIST);
    vector_add(&THREADS_NUM_LIST, &threads_num1); 
    vector_add(&THREADS_NUM_LIST, &threads_num2); 
    vector_add(&THREADS_NUM_LIST, &threads_num3); 
    vector_add(&THREADS_NUM_LIST, &threads_num4); 
    vector_add(&THREADS_NUM_LIST, &threads_num5);

    float com_time1 = 0, com_time2 = 0.000001, com_time3 = 0.00001, com_time4 = 0.0001, com_time5 = 0.001;
    vector_init(&COMPUTATION_TIME_LIST);
    vector_add(&COMPUTATION_TIME_LIST, &com_time1); vector_add(&COMPUTATION_TIME_LIST, &com_time2); 
    vector_add(&COMPUTATION_TIME_LIST, &com_time3); vector_add(&COMPUTATION_TIME_LIST, &com_time4); 
    vector_add(&COMPUTATION_TIME_LIST, &com_time5);

    vector_init(&test_time_list);

    // test settings
    int num_processes_i = 1;
    int num_processes_r = 1;
    if (argc == 2) {
        num_processes_i = atoi(argv[1]);
        num_processes_r = num_processes_i;
    }
    else if (argc == 3) {
        num_processes_i = atoi(argv[1]);
        num_processes_r = atoi(argv[2]);
    }

    load_data_from_txt();
    printf("total_size: %d\n", total_size);
    for (int i = 0; i < vector_total(&COMPUTATION_TIME_LIST); i++) {
        float comp_time = *((float *)vector_get(&COMPUTATION_TIME_LIST, i));
        sleep_time = comp_time * 1000000;
        
        for (int j = 0; j < vector_total(&THREADS_NUM_LIST); j++) {
            int thread_num = *((int *)vector_get(&THREADS_NUM_LIST, j));
            num_processes_r = num_processes_i = thread_num;
            // init setup
            root = rb_init();
            pthread_mutex_init(&show_tree_lock, NULL); // for print tree

            run_multi_thread_insert(num_processes_i);

            run_multi_thread_remove(num_processes_r);
        }

        printf("\n");
    }

    vector_clear(&THREADS_NUM_LIST);
    vector_clear(&COMPUTATION_TIME_LIST);
    vector_clear(&test_time_list);
    
    // vector_free(&THREADS_NUM_LIST);
    // vector_free(&COMPUTATION_TIME_LIST);
    // vector_free(&test_time_list);
    
    // root = rb_init();
    // pthread_mutex_init(&show_tree_lock, NULL);

    // run_serial();
    // run_multi_thread_insert(num_processes_i);
    // show_tree(root);
    // remove_dbg = true;
    // run_multi_thread_remove(num_processes_r);

    return 0;
}

void run_serial()
{
    struct timespec start, end;
    double elapsed_time;



    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < total_size; i++) {
        rb_insert(root, numbers[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1e9;
    elapsed_time += (end.tv_nsec - start.tv_nsec);
    elapsed_time *= 1e-9;
    printf("time taken by insert with 1 threads: %lfsec\n", elapsed_time);

    remove_dbg = false;
    dbg_printf("\n\n\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < total_size; i++) {
        rb_remove(root, numbers[i]);
        // show_tree(root);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1e9;
    elapsed_time += (end.tv_nsec - start.tv_nsec);
    elapsed_time *= 1e-9;
    printf("time taken by insert with 1 threads: %lfsec\n", elapsed_time);
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
        usleep(sleep_time);
        dbg_printf("[RUN] finish inserting element %d\n", element);
    }
    return NULL;
}

int run_multi_thread_insert(int thread_count)
{
    pthread_t tid[thread_count];
    size_per_thread = total_size / thread_count;

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    thread_count--; // main thread will also perform insertion
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&tid[i], NULL, run_insert, (void *)(i + 1));
    }

    run_insert(0);
    for (int i = 0; i < thread_count; i++) {
        pthread_join(tid[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1e9;
    elapsed_time += (end.tv_nsec - start.tv_nsec);
    elapsed_time *= 1e-9;
    printf("time taken by insert with %d thread and sleep %f seconds: %lf sec\n", thread_count + 1, (float)sleep_time / 1000000, elapsed_time);

    // show_tree(root);
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
        usleep(sleep_time);
        dbg_printf("[RUN] finish removing element %d\n", element);
        // show_tree(root);
    }
    return NULL;
}

int run_multi_thread_remove(int thread_count)
{
    pthread_t tid[thread_count];
    size_per_thread = total_size / thread_count;

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    thread_count--; // main thread will also perform insertion
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&tid[i], NULL, run_remove, (void *)(i + 1));
    }

    run_remove(0);
    for (int i = 0; i < thread_count; i++) {
        pthread_join(tid[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1e9;
    elapsed_time += (end.tv_nsec - start.tv_nsec);
    elapsed_time *= 1e-9;
    printf("time taken by remove with %d thread and sleep %f seconds: %lf sec\n", thread_count + 1, (float)sleep_time / 1000000, elapsed_time);

    // show_tree(root);
    return 0;
}

void load_data_from_txt()
{
    char buffer[20];
    FILE *file = fopen("data.txt", "r");
    total_size = 0;
    while (fgets(buffer, 20, file) != NULL) {
        buffer[strlen(buffer) - 1] = '\0';

        int result = strtol(buffer, NULL, 10);
        if (result > 0) {
            numbers[total_size++] = result;
        }
    }
    fclose(file);
}