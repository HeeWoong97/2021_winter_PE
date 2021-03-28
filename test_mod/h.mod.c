#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x9de7765d, "module_layout" },
	{ 0xcbf895e0, "kmalloc_caches" },
	{ 0xf9a482f9, "msleep" },
	{ 0x79aa04a2, "get_random_bytes" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x67c0c54c, "pv_ops" },
	{ 0x18372c7d, "kthread_create_on_node" },
	{ 0x25974000, "wait_for_completion" },
	{ 0xebbe12f0, "current_task" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0xc94e38f3, "kthread_stop" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x952664c5, "do_exit" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0xde6876de, "wake_up_process" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb5f17439, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x37a0cba, "kfree" },
	{ 0x608741b5, "__init_swait_queue_head" },
	{ 0xa6257a2f, "complete" },
	{ 0x5e515be6, "ktime_get_ts64" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "125E32CF5DFED92D41F0C54");
