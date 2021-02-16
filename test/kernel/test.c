#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include "calclock.h"

int a;

void print_global(void)
{
	printk("global: %d\n", a);
}

void print_local(int num)
{
	printk("local: %d\n", num);
}

int __init test_mod_init(void)
{
	struct timespec64 spclock1[2];
	struct timespec64 spclock2[2];
	unsigned long long time1, count1;
	unsigned long long time2, count2;

	int n = 1;
	a = 1;

	printk("module init...");

	ktime_get_ts64(&spclock1[0]);
	print_global();
	ktime_get_ts64(&spclock1[1]);

	ktime_get_ts64(&spclock2[0]);
	print_local(n);
	ktime_get_ts64(&spclock2[1]);

	calclock(spclock1, &time1, &count1);
	calclock(spclock2, &time2, &count2);

	printk("time taken to print global variable: %llu nsec", time1);
	printk("time taken to print local variable: %llu nsec", time2);

	return 0;
}

void __exit test_mod_exit(void)
{
	printk("module exit...");
}

module_init(test_mod_init);
module_exit(test_mod_exit);
MODULE_LICENSE("GPL");
