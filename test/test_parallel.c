#include "tree.h"
#include "vector_arr.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/ktime.h>

vector THREADS_NUM_LIST;
vector test_time_list;

int total_size = 0; 
int size_per_thread = 0;
int numbers[1000001];
tree_node *root;
int sleep_time = 0;

int finish;

extern struct mutex show_tree_lock;

void generate_data(void)
{
    int i;
    int random;
    int tmp;
    
    for (i = 0; i < 10000000; i++) {
        numbers[i] = i + 1;
        total_size++;
    }
    for (i = 0; i < 10000000 - 1; i++) {
        get_random_bytes(&random, sizeof(int));
        random %= (10000000 - 1) + i;
        tmp = numbers[i];
        numbers[i] = numbers[random];
        numbers[random] = tmp;
    }
}

int run_insert(void *data)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;

    p = numbers + ((long)data) * size_per_thread;
    start = p;
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
        rb_insert(root, element, (long)data);
    }
    finish++;

    return 0;
}

int run_multi_thread_insert(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[thread_count];
    
    size_per_thread = total_size / thread_count;

    start = ktime_get_ns();

    thread_count--;
    for (i = 0; i < thread_count; i++) {
        thread[i + 1] = kthread_run(&run_insert, (void *)(i + 1), "insert");
    }

    run_insert(0);
    while (finish != thread_count) {
        msleep(1);
    }
    finish = 0;

    end = ktime_get_ns();
    for (i = 0; i < thread_count; i++) {
        kthread_stop(thread[i + 1]);
    }

    printk("time taken by insert with %d thread: %lld nsec\n", thread_count, end - start);
    return 0;
}

int run_remove(void *data)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;

    p = numbers + ((long)data) * size_per_thread;
    start = p;
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
        rb_remove(root, element, (long)data);
    }
    finish++;

    return 0;
}

int run_multi_thread_remove(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[thread_count];
    
    size_per_thread = total_size / thread_count;

    start = ktime_get_ns();

    thread_count--;
    for (i = 0; i < thread_count; i++) {
        thread[i + 1] = kthread_run(&run_remove, (void *)(i + 1), "remove");
    }

    run_remove(0);
    while (finish != thread_count) {
        msleep(1);
    }
    finish = 0;

    end = ktime_get_ns();
    for (i = 0; i < thread_count; i++) {
        kthread_stop(thread[i + 1]);
    }

    printk("time taken by insert with %d thread: %lld nsec\n", thread_count, end - start);
    return 0;
}

void rbtree_test(void)
{
    int i;
    int threads_num1 = 1, threads_num2 = 2, threads_num3 = 4, threads_num4 = 8, threads_num5 = 16;
    int num_processes_i = 1;
    int num_processes_r = 1;
    int thread_num;

    vector_init(&THREADS_NUM_LIST);
    vector_add(&THREADS_NUM_LIST, &threads_num1); 
    vector_add(&THREADS_NUM_LIST, &threads_num2); 
    vector_add(&THREADS_NUM_LIST, &threads_num3); 
    vector_add(&THREADS_NUM_LIST, &threads_num4); 
    vector_add(&THREADS_NUM_LIST, &threads_num5);

    generate_data();
    printk("total_size: %d\n", total_size);
    for (i = 0; i < vector_total(&THREADS_NUM_LIST); i++) {
        thread_num = *((int *)vector_get(&THREADS_NUM_LIST, i));
        num_processes_r = num_processes_i = thread_num;

        root = rb_init();

        run_multi_thread_insert(num_processes_i);
        run_multi_thread_remove(num_processes_r);
    }

    printk("\n");
}


static int __init rbtree_mod_init(void)
{
    printk("%s, Entering module\n", __func__);
    rbtree_test();

    return 0;
}

static void __exit rbtree_mod_exit(void)
{
    printk("%s, Exiting module\n", __func__);
}

module_init(rbtree_mod_init);
module_exit(rbtree_mod_exit);

MODULE_LICENSE("GPL");