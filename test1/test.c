#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>

#include "tree.h"

// init

bool remove_dbg = true;

void rbtree_test(void)
{
    tree_node *root = rb_init();

    rb_insert(root, 3);
    rb_insert(root, 5);
    rb_insert(root, 15);
    rb_insert(root, 1);
    rb_insert(root, 2);
    // show_tree(root);
    rb_insert(root, 7);
    rb_insert(root, 6);
    show_tree(root);
    // rb_remove(root, 2);
    // show_tree(root);
    // rb_remove(root, 7);
    // show_tree(root);
    // rb_remove(root, 3);
    // show_tree(root);
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