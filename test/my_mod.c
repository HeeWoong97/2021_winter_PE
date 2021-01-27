#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#define dbg_printf(...) 

int __init my_mod_init(void)
{
	int i;
	for (i = 0; i < 10; i++) {
		dbg_printf("Hello Module %d\n", i);
	}

	return 0;
}

void __exit my_mod_exit(void)
{
	int i;
	for (i = 0; i < 10; i++) {
		dbg_printf("Bye Module %d\n", i);
	}
}

module_init(my_mod_init);
module_exit(my_mod_exit);
MODULE_LICENSE("GPL");
