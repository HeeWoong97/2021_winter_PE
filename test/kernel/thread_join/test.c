#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/completion.h>

struct my_thread_data
{
	struct completion *comp;
	int num;
};

int foo(void *arg)
{
	int i;
	struct my_thread_data *data = arg;

	for (i = 0; i < 10000000; i++) {
		data->num++;
	}
	printk("data->num: %d (PID: %d)", data->num, current->pid);
	printk("\n");

	complete(data->comp);

	return 0;
}

int __init test_mod_init(void)
{
	struct task_struct *threads[5];
	struct completion comps[5];
	struct my_thread_data data[5];

	int i;

	printk("module init...");

	for (i = 0; i < 5; i++) {
		init_completion(&comps[i]);
		data[i].comp = &comps[i];
		data[i].num = i;
		threads[i] = kthread_run(&foo, &data[i], "ThreadName");
		printk("data[i].num: %d (PID: %d)", data[i].num, threads[i]->pid);
	}

	for (i = 0; i < 5; i++) {
		wait_for_completion(&comps[i]);
	}

	return 0;
}

void __exit test_mod_exit(void)
{
	printk("module exit...");
}

module_init(test_mod_init);
module_exit(test_mod_exit);
MODULE_LICENSE("GPL");
