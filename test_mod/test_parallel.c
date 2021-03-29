#include "tree.h"
#include "calclock.h"

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
#include <linux/completion.h>
#include <linux/spinlock.h>

#define NUM_OF_DATA     100000

bool remove_dbg = false;

int total_size = 0; 
int size_per_thread = 0;
int numbers[NUM_OF_DATA];

tree_node *root;

int sleep_time = 0;

extern struct mutex show_tree_lock;
spinlock_t insert_lock;
// spinlock_t remove_lock;

struct my_thread_data
{
	struct completion *comp;
	int num;
};

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
	for (i = 0; i < NUM_OF_DATA - 1; i++) {
		get_random_bytes(&random, sizeof(int));
		j = random % NUM_OF_DATA;
		tmp = numbers[j];
		numbers[j] = numbers[i];
		numbers[i] = tmp;
	}
}

int run_insert(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;

    struct my_thread_data *thread_data = arg;
	struct completion *comp = thread_data->comp;
	long data = (long)thread_data->num;

    p = numbers + data * size_per_thread;
    start = p;
    count = size_per_thread;

	printk("Starting insert... (PID : %d)", current->pid);
    for (i = 0; i < count; i++) {
        element = start[i];
        spin_lock(&insert_lock);
		rb_insert(root, element, data);
		spin_unlock(&insert_lock);
    }

	complete(comp);

    return 0;
}

int run_multi_thread_insert(int thread_count, int num_of_data)
{
    int i;

    struct task_struct *thread[16];
    struct completion comps[16];
	struct my_thread_data data[16];

	struct timespec64 spclock[2];
	unsigned long long time = 0;
	unsigned long long count = 0;

    size_per_thread = num_of_data / thread_count;

	ktime_get_ts64(&spclock[0]);

    for (i = 0; i < thread_count; i++) {
        init_completion(&comps[i]);
		data[i].comp = &comps[i];
		data[i].num = i;
        thread[i] = kthread_run(&run_insert, &data[i], "insert");
    }

	for (i = 0; i < thread_count; i++) {
        wait_for_completion(&comps[i]);
    }

	ktime_get_ts64(&spclock[1]);
	calclock(spclock, &time, &count);

	for (i = 0; i < thread_count; i++) {
		kthread_stop(thread[i]);
	}

    printk("time taken by insert with %d thread: %lld nsec\n", thread_count, time);
	
    return 0;
}

int run_remove(void *arg)
{
    int *p;
    int *start;
    int count;
    int element;
    int i;

    struct my_thread_data *thread_data = arg;
	struct completion *comp = thread_data->comp;
	long data = (long)thread_data->num;

    p = numbers + data * size_per_thread;
    start = p;
    count = size_per_thread;

	printk("Starting delete... (PID : %d)", current->pid);
    for (i = 0; i < count; i++) {
        element = start[i];
        // spin_lock(&remove_lock);
        rb_remove(root, element, data);
        // spin_unlock(&remove_lock);
    }

	complete(comp);

    return 0;
}

int run_multi_thread_remove(int thread_count, int num_of_data)
{
    int i;

    struct task_struct *thread[16];
    struct completion comps[16];
	struct my_thread_data data[16];

	struct timespec64 spclock[2];
	unsigned long long time = 0;
	unsigned long long count = 0;

    size_per_thread = num_of_data / thread_count;

	ktime_get_ts64(&spclock[0]);

    for (i = 0; i < thread_count; i++) {
        init_completion(&comps[i]);
		data[i].comp = &comps[i];
		data[i].num = i;
        thread[i] = kthread_run(&run_remove, &data[i], "remove");
    }

    for (i = 0; i < thread_count; i++) {
        wait_for_completion(&comps[i]);
    }

	ktime_get_ts64(&spclock[1]);
	calclock(spclock, &time, &count);

	for (i = 0; i < thread_count; i++) {
		kthread_stop(thread[i]);
	}

    printk("time taken by remove with %d thread: %llu nsec\n", thread_count, time);

    return 0;
}

void rbtree_test(void)
{
    int i, j;
    int threads_num[3] = {1, 2, 4};
	int data[4] = {25000, 50000, 75000, 100000};
    // int data[4] = {2500, 5000, 7500, 10000};
    // int data[4] = {200, 400, 800, 1000};
	// int data[4] = {20, 40, 80, 100};
	int num_processes_i = 1;
    int num_processes_r = 1;
    int thread_num;

    generate_data();
	for (i = 0; i < 4; i++) {
		printk("total_size: %d\n", data[i]);
		for (j = 0; j < 3; j++) {
			thread_num = threads_num[j];
			// thread_num = 4;
			num_processes_r = num_processes_i = thread_num;

			root = rb_init();

			run_multi_thread_insert(num_processes_i, data[i]);
			run_multi_thread_remove(num_processes_r, data[i]);
		}
		printk("\n");
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
