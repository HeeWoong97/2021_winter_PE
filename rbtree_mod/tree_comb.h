#ifndef RED_BLACK_TREE_COMB_H
#define RED_BLACK_TREE_COMB_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/mutex.h>

// __thread long thread_index;
extern bool remove_dbg; // for only debug remove

// struct mutex show_tree_lock;

/*
#define DEBUG
#ifdef DEBUG
#define dbg_printf(fmt, ...) \
        do {                 \
            if (remove_dbg)  \
                printk("T[%ld] %s line:%d %s():" fmt, thread_index, \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        } while(0)
*/
// #else
#define dbg_printf(...)
// #endif

#define RED 0
#define BLACK 1

#define DEFAULT_MARKER -1

#define VECTOR_CAPACITY     50

typedef struct tree_node_t
{
    struct tree_node_t *parent;
    struct tree_node_t *left_child;
    struct tree_node_t *right_child;
    int value;
    char color; // RED or BLACK
    bool is_leaf;
    bool is_root;
    bool flag;
    int marker;
} tree_node;

/* vector functions */
typedef struct vector
{
    void *items[VECTOR_CAPACITY];
    int total;
} vector;

void vector_init(vector *v);
int vector_total(vector *v);
void vector_add(vector *v, void *item);
void *vector_get(vector *v, int index);
void vector_delete(vector *v, int index);
void vector_clear(vector *v);

/* function prototypes */
/* main functions */
void thread_index_init(long i);
tree_node *rb_init(void);
void right_rotate(tree_node *root, tree_node *node);
void left_rotate(tree_node *root, tree_node *node);
void tree_insert(tree_node *root, tree_node *node);
void rb_insert(tree_node *root, int value);
void rb_remove(tree_node *root, int value);
tree_node *rb_remove_fixup(tree_node *root, 
                           tree_node *node,
                           tree_node *z);
tree_node *tree_search(tree_node *root, int value);

/* utility functions  */
tree_node *create_dummy_node(void);
void show_tree_strict(tree_node *root);
void show_tree_file(tree_node *root);
void show_tree(tree_node *root);
bool check_tree(tree_node *root);
bool check_tree_dfs(tree_node *root);

bool is_root(tree_node *root, tree_node *node);
bool is_left(tree_node *node);

tree_node *create_leaf_node(void);
tree_node *get_uncle(tree_node *node);
tree_node *replace_parent(tree_node *root, tree_node *node);

void free_node(tree_node *node);

/* lock-free related */
void clear_local_area(void);
bool is_in_local_area(tree_node *target_node);

// insert related
bool setup_local_area_for_insert(tree_node *x);
tree_node *move_inserter_up(tree_node *oldx, vector *local_area);

// delete related
bool setup_local_area_for_delete(tree_node *y, tree_node *z);
tree_node *move_deleter_up(tree_node *oldx);
tree_node *par_find(tree_node *root, int value);
tree_node *par_find_successor(tree_node *delete_node);
bool release_markers_above(tree_node *start, tree_node *z);
void fix_up_case1(tree_node *x, tree_node *w);
void fix_up_case3(tree_node *x, tree_node *w);
void fix_up_case1_r(tree_node *x, tree_node *w); // mirror case
void fix_up_case3_r(tree_node *x, tree_node *w); // mirror case

#endif
