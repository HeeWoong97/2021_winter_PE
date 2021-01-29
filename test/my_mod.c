#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

int value;

void check(void)
{
	if (value == 2) return;
	printk("check function!\n");
}

void test(void)
{
	printk("test function!\n");
	check();
}

int __init my_mod_init(void)
{
	printk("Module Init\n");

	// value = 2;
	test();

	return 0;
}

void __exit my_mod_exit(void)
{
	printk("Module Exit\n");
}

module_init(my_mod_init);
module_exit(my_mod_exit);
MODULE_LICENSE("GPL");
