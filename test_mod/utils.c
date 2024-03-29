#include "tree.h"
#include "vector_arr.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/types.h>

/******************
 * helper function
 ******************/

struct mutex show_tree_lock;

/**
 * create a dummy black node, for initialization use
 */
tree_node *create_dummy_node(void)
{
    tree_node *node;
    node = (tree_node *)kmalloc(sizeof(tree_node), GFP_KERNEL);
    node->color = BLACK;
    node->value = __INT32_MAX__;
    node->left_child = create_leaf_node();
    node->right_child = create_leaf_node();
    node->is_leaf = false;
    node->parent = NULL;
    node->flag = false;
    node->marker = DEFAULT_MARKER;
    return node;
}

/**
 * create a red node, for insertion use
 */
tree_node *create_node(int value)
{
    tree_node *new_node;
    new_node = (tree_node *)kmalloc(sizeof(tree_node), GFP_KERNEL);
    new_node->color = RED;
    new_node->value = value;
    new_node->left_child = create_leaf_node();
    new_node->right_child = create_leaf_node();
    new_node->left_child->parent = new_node;
    new_node->right_child->parent = new_node;
    new_node->is_leaf = false;
    new_node->parent = NULL;
    new_node->flag = false;
    new_node->marker = DEFAULT_MARKER;
    return new_node;
}

/**
 * show tree
 * only used for debug
 * because only small tree can be shown
 */
void show_tree(tree_node *root)
{
    tree_node *root_node;
    vector frontier;

    mutex_init(&show_tree_lock);
    mutex_lock(&show_tree_lock);
    dbg_printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printk("[root] pointer: 0x%lx flag:%d\n", (unsigned long) root, (int) root->flag);

    root_node = root->left_child;
    if (root_node->is_leaf) {
        printk("Empty Tree.\n");
        mutex_unlock(&show_tree_lock);
        return;
    }
    
    vector_init(&frontier);
    vector_clear(&frontier);
    vector_add(&frontier, root_node);

    while (vector_total(&frontier) > 0) {
        tree_node *cur_node = vector_get(&frontier, vector_total(&frontier) - 1);
        tree_node *left_child = cur_node->left_child;
        tree_node *right_child = cur_node->right_child;

        printk("pointer: 0x%lx flag:%d marker: %d\n", (unsigned long) cur_node, (int) cur_node->flag, cur_node->marker);

        if (cur_node->color == BLACK)
            printk("(%d) Black\n", cur_node->value);
        else
            printk("(%d) Red\n", cur_node->value);

        vector_delete(&frontier, vector_total(&frontier) - 1);
        if (left_child->is_leaf) {
            printk("    left null flag:%d pointer: 0x%lx\n", (int)left_child->flag, (unsigned long)left_child);
        }
        else {
            if (left_child->color == BLACK)
                printk("    (%d) Black\n", left_child->value);
            else
                printk("    (%d) Red\n", left_child->value);
            vector_add(&frontier, left_child);
        }

        if (right_child->is_leaf) {
            printk("    right null flag:%d pointer: 0x%lx\n", (int)right_child->flag, (unsigned long)right_child);
        }
        else {
            if (right_child->color == BLACK)
                printk("    (%d) Black\n", right_child->value);
            else
                printk("    (%d) Red\n", right_child->value);
            vector_add(&frontier, right_child);
        }
    }
    printk("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    mutex_unlock(&show_tree_lock);
}

/**
 * show tree, will check flags and markers, for debug use
 * only used for debug
 * because only small tree can be shown
 */
void show_tree_strict(tree_node *root)
{
    tree_node *root_node;
    vector frontier;

    printk("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printk("[root] pointer: 0x%lx flag:%d\n", (unsigned long)root, (int)root->flag);

    root_node = root->left_child;
    if (root_node->is_leaf) {
        printk("Empty Tree.\n");
        return;
    }

    vector_init(&frontier);
    vector_clear(&frontier);
    vector_add(&frontier, root_node);

    while (vector_total(&frontier) > 0) {
        tree_node *cur_node = vector_get(&frontier, vector_total(&frontier) - 1);
        tree_node *left_child = cur_node->left_child;
        tree_node *right_child = cur_node->right_child;

        printk("pointer: 0x%lx flag:%d marker: %d\n", (unsigned long)cur_node, (int)cur_node->flag, cur_node->marker);
        if (cur_node->flag) 
            printk(">>>>>>> FLAG WARNING <<<<<<<\n");
        if (cur_node->marker != DEFAULT_MARKER) 
            printk(">>>>>>> MARKER WARNING <<<<<<<\n");

        if (cur_node->color == BLACK)
            printk("(%d) Black\n", cur_node->value);
        else
            printk("(%d) Red\n", cur_node->value);

        vector_delete(&frontier, vector_total(&frontier) - 1);
        if (left_child->is_leaf) {
            printk("    left null flag:%d pointer: 0x%lx\n", (int)left_child->flag, (unsigned long)left_child);
            if (cur_node->flag)
                printk(">>>>>>> FLAG WARNING <<<<<<<\n");
        }
        else {
            if (left_child->color == BLACK)
                printk("    (%d) Black\n", left_child->value);
            else
                printk("    (%d) Red\n", left_child->value);
            vector_add(&frontier, left_child);
        }

        if (right_child->is_leaf) {
            printk("    right null flag:%d pointer: 0x%lx\n", (int)right_child->flag, (unsigned long)right_child);
            if (cur_node->flag)
                printk(">>>>>>> FLAG WARNING <<<<<<<\n");
        }
        else {
            if (right_child->color == BLACK)
                printk("    (%d) Black\n", right_child->value);
            else
                printk("    (%d) Red\n", right_child->value);
            vector_add(&frontier, right_child);
        }
    }
    printk("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}

/**
 * show tree, output in file
 * only used for debug
 * because only small tree can be shown
 */
// void show_tree_file(tree_node *root)
// {
//     FILE *fd = fopen("./show_tree.txt", "w");

//     fprintk(fd, "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
//     fprintk(fd, "[root] pointer: 0x%lx flag:%d\n", (unsigned long)root, (int)root->flag);

//     tree_node *root_node = root->left_child;
//     if (root_node->is_leaf) {
//         fprintk(fd, "Empty Tree.\n");
//         fclose(fd);
//         return;
//     }

//     vector frontier;
//     vector_init(&frontier);
//     vector_clear(&frontier);
//     vector_add(&frontier, root_node);

//     while (vector_total(&frontier) > 0) {
//         tree_node *cur_node = vector_get(&frontier, vector_total(&frontier) - 1);
//         tree_node *left_child = cur_node->left_child;
//         tree_node *right_child = cur_node->right_child;

//         fprintk(fd, "pointer: 0x%lx flag:%d marker: %d\n", (unsigned long)cur_node, (int)cur_node->flag, cur_node->marker);

//         if (cur_node->color == BLACK)
//             fprintk(fd, "(%d) Black\n", cur_node->value);
//         else
//             fprintk(fd, "(%d) Red\n", cur_node->value);

//         vector_delete(&frontier, vector_total(&frontier) - 1);
//         if (left_child->is_leaf) {
//             fprintk(fd, "    left null flag:%d pointer: 0x%lx\n", (int)left_child->flag, (unsigned long)left_child);
//         }
//         else {
//             if (left_child->color == BLACK)
//                 fprintk(fd, "    (%d) Black\n", left_child->value);
//             else
//                 fprintk(fd, "    (%d) Red\n", left_child->value);
//             vector_add(&frontier, left_child);
//         }

//         if (right_child->is_leaf) {
//             fprintk(fd, "    right null flag:%d pointer: 0x%lx\n", (int)right_child->flag, (unsigned long)right_child);
//         }
//         else {
//             if (right_child->color == BLACK)
//                 fprintk(fd, "    (%d) Black\n", right_child->value);
//             else
//                 fprintk(fd, "    (%d) Red\n", right_child->value);
//             vector_add(&frontier, right_child);
//         }
//     }
//     fprintk(fd, "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
//     fclose(fd);
// }

/**
 * check tree with BFS
 */
// bool check_tree(tree_node *root)
// {
//     // rule 1,3 inherently holds
//     // rule 2
//     root = root->left_child;
//     if (root->color != BLACK) {
//         fprintk(stderr, "[ERROR] tree root with non-black color\n");
//         return false;
//     }

//     vector frontier;
//     vector_init(&frontier);
//     vector_clear(&frontier);
//     vector_add(&frontier, root);

//     while (vector_total(&frontier) > 0) {
//         tree_node *cur_node = vector_get(&frontier, vector_total(&frontier) - 1);
//         tree_node *left_child = cur_node->left_child;
//         tree_node *right_child = cur_node->right_child;

//         // rule 4
//         if (cur_node->color == RED) {
//             if (left_child->color == RED) {
//                 fprintk(stderr, "[ERROR] red node's child must be black.\n");
//                 return false;
//             }
//             if (right_child->color == RED) {
//                 fprintk(stderr, "[ERROR] red node's child must be black.\n");
//                 return false;
//             }
//         }

//         // rule 5
//         if (cur_node->color == BLACK) {
//             if (!left_child->is_leaf && !right_child->is_leaf) {
//                 if (left_child->color != right_child->color) {
//                     fprintk(stderr,
//                             "[ERROR] rule 5 violated.\n");
//                     return false;
//                 }
//             }
//             else if (left_child->is_leaf && right_child->is_leaf) {
//                 /* seems good */
//             }
//             else if (left_child->is_leaf) {
//                 /* right_child must be red and has two NULL children */
//                 if (right_child->color != RED) {
//                     fprintk(stderr,
//                             "[ERROR] rule 5 violated.\n");
//                     return false;
//                 }
//                 if (right_child->left_child || right_child->right_child) {
//                     fprintk(stderr,
//                             "[ERROR] rule 5 violated.\n");
//                     return false;
//                 }
//             }
//             else if (right_child->is_leaf) {
//                 /* left_child must be red and has two NULL children */
//                 if (left_child->color != RED) {
//                     fprintk(stderr,
//                             "[ERROR] rule 5 violated.\n");
//                     return false;
//                 }
//                 if (left_child->left_child || left_child->right_child) {
//                     fprintk(stderr,
//                             "[ERROR] rule 5 violated.\n");
//                     return false;
//                 }
//             }
//         }

//         vector_delete(&frontier, vector_total(&frontier) - 1);
//         if (!left_child->is_leaf) {
//             vector_add(&frontier, left_child);
//         }
//         if (!right_child->is_leaf) {
//             vector_add(&frontier, right_child);
//         }
//     }

//     return true;
// }

// /**
//  * check_tree_dfs_util: return the height of current node. 
//  *      compute heights of two sub-trees first.
//  *      also determine if rule 4 is violated between this node and its children.
//  *      return 0 if rule 4 or 5 is violated.
//  */
// int check_tree_dfs_util(tree_node *node)
// {
//     // no more check for leaf node
//     if (node->is_leaf) {
//         return 1;
//     }

//     // check value
//     if (!node->left_child->is_leaf && node->left_child->value > node->value) {
//         dbg_printf("[ERROR] left child's value is larger than parent's.\n");
//         return 0;
//     }
//     if (!node->right_child->is_leaf && node->right_child->value < node->value) {
//         dbg_printf("[ERROR] right child's value is smaller than parent's.\n");
//         return 0;
//     }

//     // check rule 4
//     if (node->color == RED) {
//         if (!node->left_child->is_leaf && node->left_child->color == RED) {
//             dbg_printf("[ERROR] rule 4 is violated.\n");
//             return 0;
//         }

//         if (!node->right_child->is_leaf && node->right_child->color == RED) {
//             dbg_printf("[ERROR] rule 4 is violated.\n");
//             return 0;
//         }
//     }

//     // check rule 4 and 5 recursively
//     int left_height = check_tree_dfs_util(node->left_child);
//     int right_height = check_tree_dfs_util(node->right_child);

//     // error happens in sub-tree
//     if (left_height == 0 || right_height == 0) {
//         return 0;
//     }

//     // error happens in current node
//     if (left_height != right_height) {
//         dbg_printf("[ERROR] rule 5 is violated.\n");
//         return 0;
//     }

//     return left_height + 1;
// }

// /**
//  * check_tree_dfs: check if the tree is a red-black tree. 
//  *      compute heights of two sub-trees first.
//  *      also determine if rule 4 is violated between this node and its children.
//  *      return 0 if rule 4 or 5 is violated.
//  */
// bool check_tree_dfs(tree_node *root)
// {
//     // check if root is black first
//     if (root->color != BLACK) {
//         dbg_printf("[ERROR] tree root with non-black color\n");
//         return false;
//     }

//     // no need to check children's color

//     // check value
//     if (!root->left_child->is_leaf && root->left_child->value > root->value) {
//         dbg_printf("[ERROR] left child's value is larger than root.\n");
//         return false;
//     }
//     if (!root->right_child->is_leaf && root->right_child->value < root->value) {
//         dbg_printf("[ERROR] right child's value is smaller than root.\n");
//         return false;
//     }

//     // check rule 5
//     int left_height = check_tree_dfs_util(root->left_child);
//     int right_height = check_tree_dfs_util(root->right_child);

//     if (left_height == 0 || right_height == 0 || left_height != right_height) {
//         dbg_printf("[ERROR] rule 4 or 5 is violated in left sub-tree\n");
//         return false;
//     }

//     return true;
// }

/**
 * true if node is the root node, aka has a null parent
 */
bool is_root(tree_node *root, tree_node *node)
{
    if (node->parent == root) {
        return true;
    }
    else {
        return false;
    }
}

/**
 * create a leaf node, aka null node
 */
tree_node* create_leaf_node(void)
{
    tree_node *new_node;
    new_node = (tree_node *)kmalloc(sizeof(tree_node), GFP_KERNEL);
    new_node->color = BLACK;
    new_node->value = 0;
    new_node->left_child = NULL;
    new_node->right_child = NULL;
    new_node->is_leaf = true;
    new_node->flag = false;
    new_node->marker = DEFAULT_MARKER;
    return new_node;
}

/**
 * replace the node with its child
 * this node has at most one non-nil child
 * return this child after modifying the relation ship
 */
tree_node *replace_parent(tree_node *root, tree_node *node)
{
    tree_node *child;
    if (node->left_child->is_leaf) {
        child = node->right_child;
        free_node(node->left_child);
    }
    else {
        child = node->left_child;
        free_node(node->right_child);
    }
    
    if (is_root(root, node)) {
        child->parent = root;
        root->left_child = child;
        node->parent = NULL;
    }
    
    else if (is_left(node)) {
        child->parent = node->parent;
        node->parent->left_child = child;
    }

    else {
        child->parent = node->parent;
        node->parent->right_child = child;
    }

    dbg_printf("[Remove] unlink complete.\n");
    return child;
}

/**
 * true if node is the left child of its parent node
 * false if it's right child
 */
bool is_left(tree_node *node)
{
    tree_node *parent;
    
    if (node->parent->is_leaf) {
        printk("[ERROR] root node has no parent.\n");
    }

    parent = node->parent;
    if (node == parent->left_child) {
        return true;
    }
    else {
        return false;
    }
}

/**
 * get the uncle node of current node
 */
tree_node *get_uncle(tree_node *node)
{
    tree_node *parent;
    tree_node *grand_parent;

    if (node->parent->is_leaf) {
        printk("[ERROR] get_uncle node should be at least layer 3.\n");
        return NULL;
    }

    if (node->parent->parent->is_leaf) {
        printk("[ERROR] get_uncle node should be at least layer 3.\n");
        return NULL;
    }

    parent = node->parent;
    grand_parent = parent->parent;
    if (parent == grand_parent->left_child) {
        return grand_parent->right_child;
    }
    else {
        return grand_parent->left_child;
    }
}

/**
 * free current node
 */
void free_node(tree_node *node)
{
    kfree(node);
}
