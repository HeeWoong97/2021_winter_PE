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

bool remove_dbg = false;

int total_size = 0; 
int size_per_thread = 0;
int numbers[NUM_OF_DATA];
tree_node *root;
int sleep_time = 0;

bool shuffle = true;
int finish;

extern struct mutex show_tree_lock;

void generate_data(void)
{
    int i;
    int j;
    unsigned int random;
    int tmp;
    
    for (i = 0; i < NUM_OF_DATA; i++) {
        numbers[i] = i + 1;
        total_size++;
    }
	if (shuffle) {
		for (i = 0; i < NUM_OF_DATA - 1; i++) {
			get_random_bytes(&random, sizeof(int));
			j = random % NUM_OF_DATA;
			// printk("j: %d", j);
			tmp = numbers[j];
			numbers[j] = numbers[i];
			numbers[i] = tmp;
		}
	}
}

int run_insert(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;
    long data = *(long *)arg;

    p = numbers + data * size_per_thread;
    start = p;
    // printk("*data(thread_index): %ld (PID: %d)\n", data, current->pid);
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
		// printk("insert %d (PID: %d)", element, current->pid);
        // printk("inserting element: %d (PID: %d)\n", element, current->pid);
        rb_insert(root, element, data);
        dbg_printf("[RUN] finish inserting element %d\n", element);
    }
	// printk("before finish: %d (PID: %d)", finish, current->pid);
    __sync_fetch_and_add(&finish, 1);
	// finish++;
	// printk("after finish: %d (PID: %d)", finish, current->pid);
    // printk("\n");

    return 0;
}

int run_multi_thread_insert(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[16];
    long arg[16];
    
    // printk("Enter run multi thread insert (PID: %d)", current->pid);

    size_per_thread = total_size / thread_count;

    start = ktime_get();

	finish = -thread_count;
	// finish = -4;
    // thread_count--;
    for (i = 0; i < thread_count; i++) {
        arg[i] = i;
        // printk("arg: %ld (i: %d)\n", arg[i + 1], i);
        thread[i] = kthread_run(&run_insert, &arg[i], "insert");
    }

    // printk("starting run_insert(0) (PID: %d)\n", current->pid);
    // arg[thread_count] = thread_count;
    // run_insert(&arg[thread_count]);    

    // while (__sync_fetch_and_add(&finish, 0)) {
    // while (finish != thread_count + 1) {   
	// 	msleep(1);
    // }
    // finish = -4;
	// finish = 0;
    // printk("finish sleep (PID: %d)\n", current->pid);

	while (__sync_fetch_and_add(&finish, 0)) {
		udelay(100);
	}

	if (!__sync_fetch_and_add(&finish, 0)) {
		end = ktime_get();
	}

    // end = ktime_get();

	// msleep(1);
	for (i = 0; i < thread_count; i++) {
        kthread_stop(thread[i]);
    }

    printk("time taken by insert with %d thread: %lld nsec\n", thread_count, end - start);
    // printk("\n");

    return 0;
}

int run_remove(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;
    long data = *(long *)arg;

    p = numbers + data * size_per_thread;
    start = p;
    count = size_per_thread;

    for (i = 0; i < count; i++) {
        element = start[i];
        // printk("removing element: %d (PID: %d)\n", element, current->pid);
        rb_remove(root, element, data);
        dbg_printf("[RUN] finish removing element %d\n", element);
    }
    __sync_fetch_and_add(&finish, 1);
    // printk("\n");

    return 0;
}

int run_multi_thread_remove(int thread_count)
{
    int i;
    ktime_t start, end;
    struct task_struct *thread[16];
    long arg[16];
    
    size_per_thread = total_size / thread_count;

    start = ktime_get();

	// finish = -4;
	finish = -thread_count;
    // thread_count--;
    for (i = 0; i < thread_count; i++) {
        arg[i] = i;
        thread[i] = kthread_run(&run_remove, &arg[i], "remove");
    }

    // arg[thread_count] = thread_count;
    // run_remove(&arg[thread_count]);

    // while (__sync_fetch_and_add(&finish, 0)) {
	// while (finish != thread_count + 1) { 
	// 	msleep(1);
    // }
    // finish = -4;
	// finish = 0;

	while (__sync_fetch_and_add(&finish, 0)) {
		udelay(100);
	}

	if (!__sync_fetch_and_add(&finish, 0)) {
		end = ktime_get();	
	}

    // end = ktime_get();

	// msleep(1);
    for (i = 0; i < thread_count; i++) {
        kthread_stop(thread[i]);
    }

    printk("time taken by remove with %d thread: %lld nsec\n", thread_count, end - start);
    // printk("\n");

    return 0;
}

void rbtree_test(void)
{
    int i;
    int threads_num[3] = {1, 2, 4};
    int num_processes_i = 1;
    int num_processes_r = 1;
    int thread_num;

    generate_data();
    printk("total_size: %d\n", total_size);
    for (i = 0; i < 3; i++) {
        thread_num = threads_num[i];
        // thread_num = 4;
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
