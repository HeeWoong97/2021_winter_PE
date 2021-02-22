#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>

int foo(void *arg)
{
	int i;
	int data = *(int *)arg;

	for (i = 0; i < 10000000; i++) {
		data++;
	}
	printk("data->num: %d (PID: %d)", data, current->pid);
	printk("\n");

	return 0;
}

int __init test_mod_init(void)
{
	int i;
	struct task_struct *threads[5];
	int data[5];

	printk("module init...");

	for (i = 0; i < 5; i++) {
		data[i] = i;
		threads[i] = kthread_run(&foo, &data[i], "ThreadName");
		printk("data[i].num: %d (PID: %d)", data[i], threads[i]->pid);
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
