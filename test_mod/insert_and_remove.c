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
#define NUM_OF_THREAD   4

bool remove_dbg = false;

int count;
int numbers[NUM_OF_DATA];

tree_node *root;

struct my_thread_data
{
    struct completion *comp;
    int thread_index;
};

void generate_data(void)
{
    int i, j;
    unsigned int random;
    int tmp;
    
    for (i = 0; i < NUM_OF_DATA; i++) {
        numbers[i] = i + 1;
    }
	for (i = 0; i < NUM_OF_DATA - 1; i++) {
		get_random_bytes(&random, sizeof(int));
		j = random % NUM_OF_DATA;
		tmp = numbers[j];
		numbers[j] = numbers[i];
		numbers[i] = tmp;
	}
}

// run_insert + run_remove
int thread_func(void *arg)
{  
    struct my_thread_data *thread_data = arg; 
    struct completion *comp = thread_data->comp; // for thread join
    long thread_index = (long)thread_data->thread_index; //takes thread num
    int element;
    int random;

    while (true) {
        if (count > NUM_OF_DATA) {
            break;
        }
        element = numbers[__sync_fetch_and_add(&count, 1)];
        rb_insert(root, element, thread_index);
        get_random_bytes(&random, sizeof(int));
        random %= 1000;
        udelay(random);
        rb_remove(root, element, thread_index);
    }

    complete(comp);

    return 0;
}

int run_multi_thread_insert_and_remove(int thread_count)
{
    int i;

    struct task_struct *thread[NUM_OF_THREAD];
    struct completion comps[NUM_OF_THREAD];
    struct my_thread_data data[NUM_OF_THREAD];

    struct timespec64 spclock[2];
    unsigned long long time = 0;
    unsigned long long count = 0;

    ktime_get_ts64(&spclock[0]);

    for (i = 0; i < NUM_OF_THREAD; i++) {
        init_completion(&comps[i]);
        data[i].comp = &comps[i];
        data[i].thread_index = i;
        thread[i] = kthread_run(&thread_func, &data[i], "thread_func");
    }

    for (i = 0; i < NUM_OF_THREAD; i++) {
        wait_for_completion(&comps[i]);
    }

    ktime_get_ts64(&spclock[1]);
    calclock(spclock, &time, &count);

    for (i = 0; i < NUM_OF_THREAD; i++) {
        kthread_stop(thread[i]);
    }

    printk("time taken for insert and remove: %llu nsec\n", time);

    return 0;
}

void rbtree_test(void)
{
    generate_data();
    run_multi_thread_insert_and_remove(NUM_OF_THREAD);
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