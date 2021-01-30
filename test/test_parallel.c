#include "tree.h"

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
#include <linux/sched.h>

#define NUM_OF_DATA     100000

vector THREADS_NUM_LIST;
vector test_time_list;

int total_size = 0; 
int size_per_thread = 0;
int numbers[NUM_OF_DATA];
tree_node *root;
int sleep_time = 0;

int finish;

extern struct mutex show_tree_lock;

void generate_data(void)
{
    int i;
    int random;
    int tmp;
    
    for (i = 0; i < NUM_OF_DATA; i++) {
        numbers[i] = i + 1;
        total_size++;
    }
    for (i = 0; i < NUM_OF_DATA - 1; i++) {
        get_random_bytes(&random, sizeof(int));
        random %= (NUM_OF_DATA - 1) + i;
        tmp = numbers[i];
        numbers[i] = numbers[random];
        numbers[random] = tmp;
    }
}

int run_insert(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;
    long data = (long)(int*)arg;

    p = numbers + (data) * size_per_thread;
    start = p;
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
        rb_insert(root, element, data);
    }
    finish++;

    return 0;
}

int run_multi_thread_insert(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[16];
    int arg;
    
    size_per_thread = total_size / thread_count;

    start = ktime_get_ns();

    thread_count--;
    for (i = 0; i < thread_count; i++) {
        arg = i + 1;
        thread[i + 1] = kthread_run(&run_insert, (void *)(&arg), "insert");
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

int run_remove(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;
    long data = (long)(int*)arg;

    p = numbers + (data) * size_per_thread;
    start = p;
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
        rb_remove(root, element, data);
    }
    finish++;

    return 0;
}

int run_multi_thread_remove(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[16];
    int arg;
    
    size_per_thread = total_size / thread_count;

    start = ktime_get_ns();

    thread_count--;
    for (i = 0; i < thread_count; i++) {
        arg = i + 1;
        thread[i + 1] = kthread_run(&run_remove, (void *)(&arg), "remove");
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
    int threads_num[5] = {1, 2, 4, 8, 16};
    int num_processes_i = 1;
    int num_processes_r = 1;
    int thread_num;

    generate_data();
    printk("total_size: %d\n", total_size);
    for (i = 0; i < 5; i++) {
        thread_num = threads_num[i];
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