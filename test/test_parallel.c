#include "tree.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/types.h>
// #include <linux/random.h>
#include <linux/ktime.h>
#include <linux/sched.h>

#define NUM_OF_DATA     100

vector THREADS_NUM_LIST;
vector test_time_list;

bool remove_dbg = false;

int total_size = 0; 
int size_per_thread = 0;
int numbers[100];
tree_node *root;
int sleep_time = 0;

int finish;

extern struct mutex show_tree_lock;

void generate_data(void)
{
    int i;
    // int random;
    // int tmp;
    
    for (i = 0; i < NUM_OF_DATA; i++) {
        numbers[i] = i + 1;
        total_size++;
    }
    // for (i = 0; i < NUM_OF_DATA - 1; i++) {
    //     get_random_bytes(&random, sizeof(int));
    //     random %= (NUM_OF_DATA - 1) + i;
    //     tmp = numbers[i];
    //     numbers[i] = numbers[random];
    //     numbers[random] = tmp;
    // }
}

int run_insert(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;
    long data = (long)(*(int *)arg);

    printk("*(int*)arg: %d\n", *(int *)arg);
    p = numbers + (data) * size_per_thread;
    printk("data: %ld\n", data);
    start = p;
    printk("start position: %d, size_per_thread: %d (PID: %d)\n", *p, size_per_thread, current->pid);
    count = size_per_thread;

    printk("start run_insert (PID: %d)\n", current->pid);

    for (i = 0; i < count; i++) {
        element = start[i];
        printk("inserting element: %d (PID: %d)\n", element, current->pid);
        rb_insert(root, element, data);
        dbg_printf("[RUN] finish inserting element %d\n", element);
    }
    finish++;

    return 0;
}

int run_multi_thread_insert(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[16];
    // int arg;
    
    printk("Enter run multi thread insert (PID: %d)", current->pid);

    size_per_thread = total_size / thread_count;

    start = ktime_get_ns();
    printk("ktime_get_ns finished\n");

    thread_count--;
    printk("thread count-- finished (thread_count: %d)\n", thread_count);
    for (i = 0; i < thread_count; i++) {
        printk("for loop i: %d\n", i);
        int *arg = (int *)kmalloc(sizeof(int), GFP_KERNEL);
        *arg = i + 1;
        printk("arg: %d\n", *arg);
        thread[i + 1] = kthread_run(&run_insert, (void *)arg, "insert");
        kfree(arg);
    }

    printk("starting run_insert(0)\n");
    int *arg = (int *)kmalloc(sizeof(int), GFP_KERNEL);
    *arg = 0;
    run_insert((void *)arg);
    kfree(arg);
    while (finish != thread_count + 1) {
        msleep(1);
    }
    finish = 0;
    printk("finish sleep\n");

    end = ktime_get_ns();
    for (i = 0; i < thread_count; i++) {
        kthread_stop(thread[i + 1]);
    }

    printk("time taken by insert with %d thread: %lld nsec\n", thread_count + 1, end - start);
    
    return 0;
}

int run_remove(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;
    long data = (long)(*(int *)arg);

    p = numbers + (data) * size_per_thread;
    start = p;
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
        printk("removing element: %d (PID: %d)\n", element, current->pid);
        rb_remove(root, element, data);
        dbg_printf("[RUN] finish removing element %d\n", element);
    }
    finish++;

    return 0;
}

int run_multi_thread_remove(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[16];
    // int arg;
    
    size_per_thread = total_size / thread_count;

    start = ktime_get_ns();

    thread_count--;
    for (i = 0; i < thread_count; i++) {
        // arg = i + 1;
        int *arg = (int *)kmalloc(sizeof(int), GFP_KERNEL);
        *arg = i + 1;
        thread[i + 1] = kthread_run(&run_remove, (void *)arg, "remove");
        kfree(arg);
    }

    int *arg = (int *)kmalloc(sizeof(int), GFP_KERNEL);
    *arg = 0;
    run_remove((void *)arg);
    kfree(arg);
    while (finish != thread_count + 1) {
        msleep(1);
    }
    finish = 0;

    end = ktime_get_ns();
    for (i = 0; i < thread_count; i++) {
        kthread_stop(thread[i + 1]);
    }

    printk("time taken by insert with %d thread: %lld nsec\n", thread_count + 1, end - start);
    
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
    // for (i = 0; i < 2; i++) {
        // thread_num = threads_num[i];
        thread_num = 2;
        num_processes_r = num_processes_i = thread_num;

        root = rb_init();

        run_multi_thread_insert(num_processes_i);
        run_multi_thread_remove(num_processes_r);
    // }

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