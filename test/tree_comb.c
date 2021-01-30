#include "tree_comb.h"

/*Thread Local Variable*/
long thread_index;
vector nodes_own_flag;

struct mutex show_tree_lock;

/*******************************
*         vector_arr.c
*******************************/

void vector_init(vector *v)
{
    v->total = 0;
}

int vector_total(vector *v)
{
    return v->total;
}

void vector_add(vector *v, void *item)
{
    v->items[v->total++] = item;
}

void *vector_get(vector *v, int index)
{
    if (index >= 0 && index < v->total) {
        return v->items[index];
    }
    return NULL;
}

void vector_clear(vector *v)
{
    int i;

    for (i = 0; i < v->total; i++) {
        v->items[i] = NULL;
    }
    v->total = 0;
}

void vector_delete(vector *v, int index)
{
    int i;

    if (index < 0 || index >= v->total) {
        return;
    }

    v->items[index] = NULL;

    for (i = index; i < v->total - 1; i++) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->total--;
}

/*******************************
*           tree.c
*******************************/

/**
 * initialize red-black tree and return its root
 */
tree_node *rb_init(void)
{
    tree_node *dummy1 = create_dummy_node();
    tree_node *dummy2 = create_dummy_node();
    tree_node *dummy3 = create_dummy_node();
    tree_node *dummy4 = create_dummy_node();
    tree_node *dummy5 = create_dummy_node();
    tree_node *dummy_sibling = create_dummy_node();
    tree_node *root = create_dummy_node();

    dummy_sibling->parent = root;
    root->parent = dummy5;
    dummy5->parent = dummy4;
    dummy4->parent = dummy3;
    dummy3->parent = dummy2;
    dummy2->parent = dummy1;

    free_node(dummy1->left_child);
    dummy1->left_child = dummy2;
    free_node(dummy2->left_child);
    dummy2->left_child = dummy3;
    free_node(dummy3->left_child);
    dummy3->left_child = dummy4;
    free_node(dummy4->left_child);
    dummy4->left_child = dummy5;
    free_node(dummy5->left_child);
    dummy5->left_child = root;
    free_node(root->right_child);
    root->right_child = dummy_sibling;
    return root;
}

/**
 * perform red-black left rotation
 */
void left_rotate(tree_node *root, tree_node *node)
{
    tree_node *right_child;

    if (node->is_leaf) {
        printk("[ERROR] invalid rotate on NULL node.\n");
        do_exit(1);
    }
    
    if (node->right_child->is_leaf) {
        printk("[ERROR] invalid rotate on node with NULL right child.\n");
        do_exit(1);
    }

    right_child = node->right_child;
    right_child->parent = node->parent;
    if (is_left(node)) {
        node->parent->left_child = right_child;
    }
    else {
        node->parent->right_child = right_child;
    }

    node->parent = right_child;

    node->right_child = right_child->left_child;
    right_child->left_child = node;

    if (node->right_child) {
        node->right_child->parent = node;
    }
    
    dbg_printf("[Rotate] Left rotation complete.\n");
}

/**
 * perform red-black right rotation
 */
void right_rotate(tree_node *root, tree_node *node)
{
    tree_node *left_child;

    if (node->is_leaf) {
        printk("[ERROR] invalid rotate on NULL node.\n");
        do_exit(1);
    }

    if (node->left_child->is_leaf) {
        printk("[ERROR] invalid rotate on node with NULL left child.\n");
        do_exit(1);
    }

    left_child = node->left_child;
    left_child->parent = node->parent;
    if (is_left(node)) {
        node->parent->left_child = left_child;
    }
    else {
        node->parent->right_child = left_child;
    }
    
    node->parent = left_child;

    node->left_child = left_child->right_child;
    left_child->right_child = node;

    if (node->left_child) {
        node->left_child->parent = node;
    }

    dbg_printf("[Rotate] Right rotation complete.\n");
}

/**
 * basic insertion of a binary search tree
 * further fixup needed for red-black tree
 */
void tree_insert(tree_node *root, tree_node *new_node)
{
    tree_node *z;
    tree_node *curr_node;
    int value = new_node->value;

    // insert like any binary search tree
    bool expected = false;
    while (!__sync_bool_compare_and_swap(&root->flag, expected, true));

    dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)root);

    // empty tree
    if (root->left_child->is_leaf) {
        free_node(root->left_child);
        new_node->flag = true;
        dbg_printf("[FLAG] set flag of 0x%lx\n", (unsigned long)new_node);
        root->left_child = new_node;
        new_node->parent = root;
        dbg_printf("[Insert] new node with value (%d)\n", value);
        dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)root);
        root->flag = false;
        return;
    }

    // release root's flag for non-empty tree
    dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)root);
    root->flag = false;

restart:

    z = NULL;
    curr_node = root->left_child;
    expected = false;
    if (!__sync_bool_compare_and_swap(&curr_node->flag, expected, true)) {
        dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)curr_node);
        
        goto restart;
    }
    dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)curr_node);

    
    while (!curr_node->is_leaf) {
        z = curr_node;
        if (value > curr_node->value) { /* go right */
            curr_node = curr_node->right_child;
        }
        else { /* go left */
            curr_node = curr_node->left_child;
        }

        expected = false;
        if (!__sync_bool_compare_and_swap(&curr_node->flag, expected, true)) {
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)curr_node);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)z);
            z->flag = false;// release z's flag
            
            goto restart;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)curr_node);

        if (!curr_node->is_leaf) {
            // release old curr_node's flag
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)z);
            z->flag = false;
        }
    }
    
    new_node->flag = true;
    if (!setup_local_area_for_insert(z)) {
        curr_node->flag = false;
        dbg_printf("[FLAG] release flag of %lu and %lu\n", (unsigned long)z, (unsigned long)curr_node);
        z->flag = false;
        goto restart;
    }

    // now the local area has been setup
    // insert the node
    new_node->parent = z;
    if (value <= z->value) {
        kfree(z->left_child);
        z->left_child = new_node;
    }
    else {
        kfree(z->right_child);
        z->right_child = new_node;
    }
    
    dbg_printf("[Insert] new node with value (%d)\n", value);
}

/**
 * insert a new node
 * fixup the tree to be a red-black tree
 */
void rb_insert(tree_node *root, int value)
{
    tree_node *new_node;
    tree_node *curr_node;
    tree_node *parent, *uncle, *grandparent;
    vector local_area;
    int i;

    // init thread local nodes with flag
    clear_local_area();

    new_node = (tree_node *)kmalloc(sizeof(tree_node), GFP_KERNEL);
    new_node->color = RED;
    new_node->value = value;
    new_node->left_child = create_leaf_node();
    new_node->right_child = create_leaf_node();
    new_node->is_leaf = false;
    new_node->parent = NULL;
    new_node->flag = false;
    new_node->marker = DEFAULT_MARKER;

    tree_insert(root, new_node); // normal insert
    
    curr_node = new_node;

    uncle = NULL; 
    grandparent = NULL;

    parent = curr_node->parent;
    vector_init(&local_area);
    vector_add(&local_area, curr_node);
    vector_add(&local_area, parent);

    if (parent != NULL) {
        grandparent = parent->parent; 
    }

    if (grandparent != NULL) {
        if (grandparent->left_child == parent) {
            uncle = grandparent->right_child;
        }
        else {
            uncle = grandparent->left_child;
        }
    }

    vector_add(&local_area, uncle);
    vector_add(&local_area, grandparent);

    if (is_root(root, curr_node)) {
        curr_node->color = BLACK;
        for (i = 0; i < vector_total(&local_area); i++) {
            tree_node *node = vector_get(&local_area, i);
            if (node != NULL)  {
                dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)node);
                node->flag = false;
            }
        }
        dbg_printf("[INSERT] insertFixup complete.\n");
        return;
    }

    while (true) {
        if (is_root(root, curr_node)) { // trivial case 1
            curr_node->color = BLACK;
            break;
        }
        
        parent = curr_node->parent;

        if (parent->color == BLACK) { // trivial case 2
            break;
        }
        
        uncle = get_uncle(curr_node);

        if (parent->color == RED && uncle->color == RED) { /* case 1 */
            parent->color = BLACK;
            uncle->color = BLACK;
            // curr_node = parent->parent;
            parent->parent->color = RED;

            curr_node = move_inserter_up(curr_node, &local_area);
            continue;
        }

        if (is_left(parent)) {
            switch (is_left(curr_node)) {
            case false:
                left_rotate(root, parent);
                curr_node = parent;
            case true:
                parent = curr_node->parent;
                uncle = get_uncle(curr_node);

                parent->parent->color = RED;
                parent->color = BLACK;
                right_rotate(root, parent->parent);
                break;
            }
            break;
        }
        else { // parent is the right child of its parent
            switch (is_left(curr_node)) {
            case true:
                right_rotate(root, parent);
                curr_node = parent;
            case false:
                parent = curr_node->parent;
                uncle = get_uncle(curr_node);

                parent->parent->color = RED;
                parent->color = BLACK;
                left_rotate(root, parent->parent);
                break;
            }
            break;
        }
    }

    // release flags of all nodes in local_area
    for (i = 0; i < vector_total(&local_area); i++) {
        tree_node *node = vector_get(&local_area, i);
        if (node != NULL) {
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)node);
            node->flag = false;
        }
    }
    
    dbg_printf("[Insert] rb fixup complete.\n");
    vector_clear(&local_area);
    // vector_free(&local_area);
}

/**
 * red-black tree remove
 */
void rb_remove(tree_node *root, int value)
{
    tree_node *z;
    tree_node *y; // actual delete node
    tree_node *replace_node;

    dbg_printf("[Remove] thread %ld value %d\n", thread_index, value);
    // init thread local nodes with flag
    clear_local_area();
restart:

    z = par_find(root, value);
    if (z == NULL)
        return;

    if (z->left_child->is_leaf || z->right_child->is_leaf)
        y = z;
    else
        y = par_find_successor(z);
    
    if (y == NULL) {
        z->flag = false;
        goto restart;
    }
    
    // we now hold the flag of y(delete_node) AND of z(node)

    // set up for local area delete, also four markers above
    if (!setup_local_area_for_delete(y, z)) {
        // release flags
        y->flag = false;
        if (y != z) z->flag = false;
        goto restart; // deletion failed, try again
    }
    dbg_printf("[Remove] actual node with value %d\n", y->value);
    
    // unlink y from the tree
    replace_node = replace_parent(root, y);

    // replace the value
    if (y != z)
        z->value = y->value;
    
    // release z's flag safely
    if (!is_in_local_area(z)) {
        z->flag = false;
        dbg_printf("[Flag] release %d\n", z->value);
    }

    if (y->color == BLACK) /* fixup case */
        replace_node = rb_remove_fixup(root, replace_node, z);
    
    // clear markers above
    while (!release_markers_above(replace_node->parent, z))
        ;

    clear_local_area();
    
    dbg_printf("[Remove] node with value %d complete.\n", value);
    free_node(y);
}

/**
 * red-black tree remove fixup
 * when delete node and replace node are both black
 * rule 5 is violated and should be fixed
 */
tree_node *rb_remove_fixup(tree_node *root, 
                           tree_node *node,
                           tree_node *z)
{
    while (!is_root(root, node) && node->color == BLACK) {
        tree_node *brother_node;
        if (is_left(node)) {
            brother_node = node->parent->right_child;
            if (brother_node->color == RED) { // case 1
                brother_node->color = BLACK;
                node->parent->color = RED;
                left_rotate(root, node->parent);
                brother_node = node->parent->right_child; // must be black

                fix_up_case1(node, brother_node);
                dbg_printf("[Remove] case1 done.\n");
            } // case 1 will definitely turn into case 2

            if (brother_node->left_child->color == BLACK &&
                brother_node->right_child->color == BLACK) { // case 2
                brother_node->color = RED;
                node = move_deleter_up(node);
                dbg_printf("[Remove] case2 done.\n");
            }

            else if (brother_node->right_child->color == BLACK) { // case 3
                brother_node->left_child->color = BLACK;
                brother_node->color = RED;
                right_rotate(root, brother_node);
                brother_node = node->parent->right_child;

                fix_up_case3(node, brother_node);
                dbg_printf("[Remove] case3 done.\n");
            }

            else { // case 4
                brother_node->color = node->parent->color;
                node->parent->color = BLACK;
                brother_node->right_child->color = BLACK;
                left_rotate(root, node->parent);

                node = node->parent;
                dbg_printf("[Remove] case4 done.\n");
                break;
            }
        }
        else { // mirror case of the above
            brother_node = node->parent->left_child;
            if (brother_node->color == RED) {
                brother_node->color = BLACK;
                node->parent->color = RED;
                right_rotate(root, node->parent);
                brother_node = node->parent->left_child;

                fix_up_case1_r(node, brother_node);
                dbg_printf("[Remove] case1 done.\n");
            }

            if (brother_node->left_child->color == BLACK &&
                     brother_node->right_child->color == BLACK) {
                brother_node->color = RED;
                node = move_deleter_up(node);
                dbg_printf("[Remove] case2 done.\n");
            }

            else if (brother_node->left_child->color == BLACK) { // case 3
                brother_node->right_child->color = BLACK;
                brother_node->color = RED;
                left_rotate(root, brother_node);
                brother_node = node->parent->left_child;

                fix_up_case3_r(node, brother_node);
                dbg_printf("[Remove] case3 done.\n");
            }

            else { // case 4
                brother_node->color = node->parent->color;
                node->parent->color = BLACK;
                brother_node->left_child->color = BLACK;
                right_rotate(root, node->parent);

                node = node->parent;
                dbg_printf("[Remove] case4 done.\n");
                break;
            }
        }
    }

    node->color = BLACK;
    
    dbg_printf("[Remove] fixup complete.\n");
    return node;
}

/**
 * lock-free tree search
 */
tree_node *tree_search(tree_node *root, int value)
{
    tree_node *z = par_find(root, value);

    dbg_printf("[Warning] tree serach not found.\n");
    return z;
}

/*******************************
*           utils.c
*******************************/

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

/*******************************
*       lockfree_utils.c
*******************************/

/**
 * cannot find a 'extern' way to use thread_local variables
 * across files
 */
void thread_index_init(long i)
{
    thread_index = i;
}

/**
 * clear local area
 */
void clear_local_area(void)
{   
    int i;
    tree_node *node;

    if (vector_total(&nodes_own_flag) == 0) return;
    dbg_printf("[Flag] Clear\n");
    for (i = 0; i < vector_total(&nodes_own_flag); i++) {
        node = vector_get(&nodes_own_flag, i);
        // node->flag = false; // ??
        dbg_printf("[Flag]      %d, 0x%lx, %d\n",
                node->value, (unsigned long)node, (int)node->flag);
    }
    vector_clear(&nodes_own_flag);
}

/**
 * true if the node is in our local area
 */
bool is_in_local_area(tree_node *target_node)
{
    int i;

    for (i = 0; i < vector_total(&nodes_own_flag); i++) {
        tree_node *node = vector_get(&nodes_own_flag, i);
        // if (node != NULL) {
            if (node == target_node) return true;        
        // }
    }
    
    return false;
}

/**
 * return true if the node has no other's marker
 * 
 * z - the node returned by par_find()
 */
bool has_no_others_marker(tree_node *t, tree_node *z,
                                int TID_to_ignore)
{
    // We hold flags on both t and z.
    // check that t has no marker set
    if (t != z && t->marker != DEFAULT_MARKER && t->marker != TID_to_ignore) 
        return false;
    
    return true;
}

/**
 * try to get four markers above by getting flags first
 * 
 * @params:
 *      start - parent node of the actual node to be deleted
 */
bool get_markers_above(tree_node *start, tree_node *z, bool release)
{
    bool expect;

    // Now get marker(s) above
    tree_node *pos1, *pos2, *pos3, *pos4;

    pos1 = start->parent;
    if (pos1 != z) {
        expect = false;
        if (!__sync_bool_compare_and_swap(&pos1->flag, expect, true))
            return false;
    }
    
    if ((pos1 != start->parent) 
        || (!has_no_others_marker(pos1, z, thread_index))) {
        if (pos1 != z) 
            pos1->flag = false;
        return false;
    }

    pos2 = pos1->parent;
    if (pos2 != z) {
        expect = false;
        if (!__sync_bool_compare_and_swap(&pos2->flag, expect, true)) {
            if (pos1 != z)
                pos1->flag = false;
            return false;
        }
    }

    if ((pos2 != pos1->parent) 
        || (!has_no_others_marker(pos2, z, thread_index))) {
        if (pos1 != z)
            pos1->flag = false;
        if (pos2 != z)
            pos2->flag = false;
        return false;
    }

    pos3 = pos2->parent;
    if (pos3 != z) {
        expect = false;
        if (!__sync_bool_compare_and_swap(&pos3->flag, expect, true)) {
            if (pos1 != z)
                pos1->flag = false;
            if (pos2 != z)
                pos2->flag = false;
            return false;
        }
    }

    if ((pos3 != pos2->parent) 
        || (!has_no_others_marker(pos3, z, thread_index))) {
        if (pos1 != z)
            pos1->flag = false;
        if (pos2 != z)
            pos2->flag = false;
        if (pos3 != z)
            pos3->flag = false;
        return false;
    }

    pos4 = pos3->parent;
    if (pos4 != z) {
        expect = false;
        if (!__sync_bool_compare_and_swap(&pos4->flag, expect, true)) {
            if (pos1 != z)
                pos1->flag = false;
            if (pos2 != z)
                pos2->flag = false;
            if (pos3 != z)
                pos3->flag = false;
            return false;
        }
    }
    
    if ((pos4 != pos3->parent) 
        || (!has_no_others_marker(pos4, z, thread_index))) {
        if (pos1 != z)
            pos1->flag = false;
        if (pos2 != z)
            pos2->flag = false;
        if (pos3 != z)
            pos3->flag = false;
        if (pos4 != z)
            pos4->flag = false;
        return false;
    }

    // successfully get the four markers
    pos1->marker = thread_index;
    pos2->marker = thread_index;
    pos3->marker = thread_index;
    pos4->marker = thread_index;

    if (release) {
        if (pos1 != z)
            pos1->flag = false;
        if (pos2 != z)
            pos2->flag = false;
        if (pos3 != z)
            pos3->flag = false;
        if (pos4 != z)
            pos4->flag = false;
    }

    dbg_printf("[Flag] get for marker: %d %d %d %d\n",
               pos1->value, pos2->value, pos3->value, pos4->value);

    return true;
}

/**
 * @params
 *  z: target node (replace value)
 *  y: actually delete node
 */
bool setup_local_area_for_delete(tree_node *y, tree_node *z)
{
    bool expect;
    tree_node *x;
    tree_node *yp;
    tree_node *w;
    tree_node *wlc, *wrc;

    // the replace child, the actual target node
    x = y->left_child;
    if (y->left_child->is_leaf)
        x = y->right_child;
    
    expect = false;
    // Try to get flags for the rest of the local area
    if (!__sync_bool_compare_and_swap(&x->flag, expect, true)) return false;
    
    yp = y->parent; // keep a copy of our parent pointer
    expect = false;
    if ((yp != z) && (!__sync_bool_compare_and_swap(&yp->flag, expect, true))) {
        x->flag = false;
        return false;
    }
    if (yp != y->parent) { // verify that parent is unchanged  
        x->flag = false; 
        if (yp!=z) yp->flag = false;
        return false;
    }
    w = y->parent->left_child;
    if (is_left(y))
        w = y->parent->right_child;
    
    expect = false;
    if (!__sync_bool_compare_and_swap(&w->flag, expect, true)) {
        x->flag = false;
        if (yp != z)
            yp->flag = false;
        return false;
    }

    if (!w->is_leaf) {
        wlc = w->left_child;
        wrc = w->right_child;

        expect = false;
        if (!__sync_bool_compare_and_swap(&wlc->flag, expect, true)) {
            x->flag = false;
            w->flag = false;
            if (yp != z)
                yp->flag = false;
            return false;
        }
        expect = false;
        if (!__sync_bool_compare_and_swap(&wrc->flag, expect, true)) {
            x->flag = false;
            w->flag = false;
            wlc->flag = false;
            if (yp != z)
                yp->flag = false;
            return false;
        }
    }

    // get four markers above to keep distance with other threads
    if (!get_markers_above(yp, z, true)) {
        x->flag = false;
        w->flag = false;
        if (!w->is_leaf) {
            wlc->flag = false;
            wrc->flag = false;
        }
        if (yp != z)
            yp->flag = false;
        return false;
    }

    // local area setup
    vector_init(&nodes_own_flag);
    vector_add(&nodes_own_flag, x);
    vector_add(&nodes_own_flag, w);
    vector_add(&nodes_own_flag, yp);
    if (!w->is_leaf) {
        vector_add(&nodes_own_flag, wlc);
        vector_add(&nodes_own_flag, wrc);
        dbg_printf("[Flag] local area: %d %d %d %d %d\n",
                   x->value, w->value, yp->value, wlc->value, wrc->value);
    }
    else {
        dbg_printf("[Flag] local area: %d %d %d\n",
                   x->value, w->value, yp->value);
    }

    return true;
}

/**
 * add intention markers (four is sufficient) as needed
 * and additional one (remove moveUp) or two (insert moveUp) markers
 */
bool get_flags_and_markers_above(tree_node *start, int numAdditional)
{
    /**
     * Check for a moveUpStruct provided by another process (due to 
     * Move-Up rule processing) and set ’PIDtoIgnore’ to the PID 
     * provided in that structure. Use the ’IsIn’ function to determine 
     * if a node is in the moveUpStruct. 
     * Start by getting flags on the four nodes we have markers on.
     */
    bool expect;
    tree_node *pos1;
    tree_node *pos2;
    tree_node *pos3;
    tree_node *pos4;
    tree_node *firstnew;
    tree_node *secondnew;

    // get markers first and do not release flag
    if (!get_markers_above(start, NULL, false))
        return false;

    pos1 = start->parent;
    pos2 = pos1->parent;
    pos3 = pos2->parent;
    pos4 = pos3->parent;
    // no need of this function
    // if (!get_flags_for_markers(start, moveUpStruct, &pos1, &pos2, &pos3, &pos4))
    //     return false;

    // Now get additional marker(s) above
    firstnew = pos4->parent;
    expect = false;
    if (!__sync_bool_compare_and_swap(&firstnew->flag, expect, true)) {
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        pos4->flag = false;
        return false;
    }

    if ((firstnew != pos4->parent) 
        || (!has_no_others_marker(firstnew, start, thread_index))) {
        firstnew->flag = false;
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        pos4->flag = false;
        return false;
    }

    dbg_printf("[Flag] firstnew: %d\n",
               firstnew->value);

    secondnew = NULL;
    if (numAdditional == 2) { // insertion so need another marker
        secondnew = firstnew->parent;
        expect = false;
        if (!__sync_bool_compare_and_swap(&secondnew->flag, expect, true)) {
            firstnew->flag = false;
            pos1->flag = false;
            pos2->flag = false;
            pos3->flag = false;
            pos4->flag = false;
            return false;
        }

        if ((secondnew != firstnew->parent) 
            || (!has_no_others_marker(secondnew, start, thread_index))) {
            secondnew->flag = false;
            firstnew->flag = false;
            pos1->flag = false;
            pos2->flag = false;
            pos3->flag = false;
            pos4->flag = false;
            return false;
        }
        dbg_printf("[Flag] second new: %d\n",
                   secondnew->value);
    }

    firstnew->marker = thread_index;
    if (numAdditional == 2)
        secondnew->marker = thread_index;

    // release the four topmost flags acquired to extend markers.
    // This leaves flags on nodes now in the new local area.
    if (numAdditional == 2)
        secondnew->flag = false;

    firstnew->flag = false;
    pos4->flag = false;
    pos3->flag = false;

    if (numAdditional == 1)
        pos2->flag = false;
    return true;
}

/**
 * try to release four markers above
 * this is the anti-function of get_markers_above()
 * this should always be valid if the rotation fixups are doing well
 * 
 * @params:
 *      start - parent node of the actual node to be deleted
 *      z - the node returned by par_find()
 */
bool release_markers_above(tree_node *start, tree_node *z)
{
    bool expect;
    
    // release 4 marker(s) above start node
    tree_node *pos1, *pos2, *pos3, *pos4;

    pos1 = start->parent;
    expect = false;
    if (!__sync_bool_compare_and_swap(&pos1->flag, expect, true))
        return false;
    if (pos1 != start->parent) { // verify that parent is unchanged
        pos1->flag = false;
        return false;
    }
    pos2 = pos1->parent;
    expect = false;
    if (!__sync_bool_compare_and_swap(&pos2->flag, expect, true)) {
        pos1->flag = false;
        return false;
    }
    if (pos2 != pos1->parent) { // verify that parent is unchanged 
        pos1->flag = false;
        pos2->flag = false;
        return false;
    }
    pos3 = pos2->parent;
    expect = false;
    if (!__sync_bool_compare_and_swap(&pos3->flag, expect, true)) {
        pos1->flag = false;
        pos2->flag = false;
        return false;
    }
    if (pos3 != pos2->parent) { // verify that parent is unchanged
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        return false;
    }
    pos4 = pos3->parent;
    expect = false;
    if (!__sync_bool_compare_and_swap(&pos4->flag, expect, true)) {
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        return false;
    }
    if (pos4 != pos3->parent) { // verify that parent is unchanged
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        pos4->flag = false;
        return false;
    }

    // release these markers
    if (pos1->marker == thread_index) pos1->marker = DEFAULT_MARKER;
    if (pos2->marker == thread_index) pos2->marker = DEFAULT_MARKER;
    if (pos3->marker == thread_index) pos3->marker = DEFAULT_MARKER;
    if (pos4->marker == thread_index) pos4->marker = DEFAULT_MARKER;

    dbg_printf("[Marker] release markers %d %d %d %d\n",
                pos1->value, pos2->value, pos3->value, pos4->value);
    
    // release flags
    pos1->flag = false;
    pos2->flag = false;
    pos3->flag = false;
    pos4->flag = false;

    return true;
}

/**
 * move a deleter up the tree
 * case 2 in deletion
 */
tree_node *move_deleter_up(tree_node *oldx)
{
    bool expect;
    tree_node *oldp;
    tree_node *oldw;
    tree_node *oldwlc;
    tree_node *oldwrc;
    tree_node *newx;
    tree_node *newp;
    tree_node *neww, *newwlc, *newwrc;

    // get direct pointers
    oldp = oldx->parent;
    oldw = oldp->left_child;

    if (is_left(oldx))
        oldw = oldp->right_child;

    oldwlc = oldw->left_child;
    oldwrc = oldw->right_child;

    // extend intention markers (getting flags to set them)
    // from oldgp to top and one more. Also convert marker on oldgp to a flag
    while (!get_flags_and_markers_above(oldp, 1))
        ;
    
    // get flags on the rest of new local area (w, wlc, wrc)
    newx = oldp;
    newp = newx->parent;

restart:
    neww = newp->left_child;
    if (is_left(newx))
        neww = newp->right_child;
    
    expect = false;
    if (!__sync_bool_compare_and_swap(&neww->flag, expect, true)) {
        goto restart;
    }
    
    newwlc = neww->left_child;
    newwrc = neww->right_child;

    expect = false;
    if (!__sync_bool_compare_and_swap(&newwlc->flag, expect, true)) {
        neww->flag = false;
        goto restart;
    }

    expect = false;
    if (!__sync_bool_compare_and_swap(&newwrc->flag, expect, true)) {
        newwlc->flag = false;
        neww->flag = false;
        goto restart;
    }

    // release flags on old local area
    oldx->flag = false;
    oldw->flag = false;
    oldwlc->flag = false;
    oldwrc->flag = false;

    dbg_printf("[Flag] release old local area: %d %d %d %d\n",
                oldx->value, oldw->value, oldwlc->value, oldwrc->value);

    // clear marker
    newx->parent->marker = DEFAULT_MARKER;

    // new local area
    vector_clear(&nodes_own_flag);
    vector_add(&nodes_own_flag, newx);
    vector_add(&nodes_own_flag, neww);
    vector_add(&nodes_own_flag, newp);
    vector_add(&nodes_own_flag, newwlc);
    vector_add(&nodes_own_flag, newwrc);
    dbg_printf("[Flag] get new local area: %d %d %d %d %d\n",
               newx->value, neww->value, newp->value, 
               newwlc->value, newwrc->value);
    return newx;
}


/**
 * fix the side effect of delete case 1
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation 
 *    --> clear all the 'naughty' markers within old local area
 */
void fix_up_case1(tree_node *x, tree_node *w)
{
    tree_node *oldw = x->parent->parent;
    tree_node *oldwlc = x->parent->right_child;
    tree_node *oldwrc = oldw->right_child;

    // clear markers
    if (oldw->marker != DEFAULT_MARKER && oldw->marker == oldwlc->marker) {
        x->parent->marker = oldw->marker;
    }

    // set w's marker before releasing its flag
    oldw->marker = thread_index;
    oldw->flag = false;
    oldwrc->flag = false;
    dbg_printf("[Flag] release %d %d\n", oldw->value, oldwrc->value);

    // release the fifth marker
    oldw->parent->parent->parent->parent->marker = DEFAULT_MARKER;

    // get the flag of the new wlc & wrc
    // this will always be valid because of markers
    // which means others may hold markers on them, but no flags on them
    w->left_child->flag = true;
    w->right_child->flag = true;
    dbg_printf("[Flag] get new %d %d\n", 
                w->left_child->value, w->right_child->value);

    // new local area
    vector_clear(&nodes_own_flag);
    vector_add(&nodes_own_flag, x);
    vector_add(&nodes_own_flag, x->parent);
    vector_add(&nodes_own_flag, w);
    vector_add(&nodes_own_flag, w->left_child);
    vector_add(&nodes_own_flag, w->right_child);
}

/**
 * fix the side effect of delete case 3
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation
 * layout:
 *    2
 * 1     3
 *     4   5
 *    6 7
 */
void fix_up_case3(tree_node *x, tree_node *w)
{
    int i;
    tree_node *oldw = w->right_child;
    tree_node *oldwrc = oldw->right_child;

    // clear all the markers within old local area
    for (i = 0; i < vector_total(&nodes_own_flag); i++) {
        tree_node *node = vector_get(&nodes_own_flag, i);
        // if (node != NULL) {
            // node->marker = DEFAULT_MARKER;
            node->marker = DEFAULT_MARKER;
        // }
    }
    // get the flag of the new wlc & wrc
    // this will always be valid because of markers
    // which means others may hold markers on them, but no flags on them
    w->left_child->flag = true;
    oldwrc->flag = false;
    dbg_printf("[Flag] release %d, get %d\n", 
                oldwrc->value, w->left_child->value);

    // new local area
    vector_clear(&nodes_own_flag);
    vector_add(&nodes_own_flag, x);
    vector_add(&nodes_own_flag, x->parent);
    vector_add(&nodes_own_flag, w);
    vector_add(&nodes_own_flag, w->left_child);
    vector_add(&nodes_own_flag, oldw);
}

/**
 * fix the side effect of delete case 1 - mirror case
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation 
 *    --> clear all the 'naughty' markers within old local area
 */
void fix_up_case1_r(tree_node *x, tree_node *w)
{
    tree_node *oldw = x->parent->parent;
    tree_node *oldwlc = oldw->left_child;
    tree_node *oldwrc = x->parent->left_child;

    // clear markers
    if (oldw->marker != DEFAULT_MARKER && oldw->marker == oldwrc->marker) {
        x->parent->marker = oldw->marker;
    }

    // set w's marker before releasing its flag
    oldw->marker = thread_index;
    oldw->flag = false;
    oldwlc->flag = false;
    dbg_printf("[Flag] release %d %d\n",
               oldw->value, oldwlc->value);
    // release the fifth marker
    oldw->parent->parent->parent->parent->marker = DEFAULT_MARKER;

    // get the flag of the new wlc & wrc
    // this will always be valid because of markers
    // which means others may hold markers on them, but no flags on them
    w->left_child->flag = true;
    w->right_child->flag = true;
    dbg_printf("[Flag] get new %d %d\n",
               w->left_child->value, w->right_child->value);
    // new local area
    vector_clear(&nodes_own_flag);
    vector_add(&nodes_own_flag, x);
    vector_add(&nodes_own_flag, x->parent);
    vector_add(&nodes_own_flag, w);
    vector_add(&nodes_own_flag, w->left_child);
    vector_add(&nodes_own_flag, w->right_child);
}

/**
 * fix the side effect of delete case 3 - mirror case
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation
 */
void fix_up_case3_r(tree_node *x, tree_node *w)
{
    int i;
    tree_node *oldw = w->left_child;
    tree_node *oldwlc = oldw->left_child;

    // clear all the markers within old local area
    for (i = 0; i < vector_total(&nodes_own_flag); i++) {
        tree_node *node = vector_get(&nodes_own_flag, i);
        // if (node != NULL) {
            // node->marker = DEFAULT_MARKER;
            node->marker = DEFAULT_MARKER;
        // }
    }

    // get the flag of the new wlc & wrc
    // this will always be valid because of markers
    // which means others may hold markers on them, but no flags on them
    w->right_child->flag = true;
    oldwlc->flag = false;
    dbg_printf("[Flag] release %d, get %d\n",
               oldwlc->value, w->right_child->value);
    // new local area
    vector_clear(&nodes_own_flag);
    vector_add(&nodes_own_flag, x);
    vector_add(&nodes_own_flag, x->parent);
    vector_add(&nodes_own_flag, w);
    vector_add(&nodes_own_flag, oldw);
    vector_add(&nodes_own_flag, w->right_child);
}

/************************ insert ************************/
/**
 * Get flags in local area
 */
bool setup_local_area_for_insert(tree_node *x)
{
    bool expected;
    tree_node *parent = x->parent;
    tree_node *uncle = NULL;

    if (parent == NULL)
        return true;

    expected = false;
    if (!__sync_bool_compare_and_swap(&parent->flag, expected, true)) {
        dbg_printf("[FLAG] failed getting flag of 0x%lx\n", 
                   (unsigned long)parent);
        return false;
    }

    dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)parent);

    // abort when parent of x changes
    if (parent != x->parent) {
        dbg_printf("[FLAG] parent changed from %lu to 0x%lx\n", 
                   (unsigned long)parent, (unsigned long)parent);
        dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)parent);
        parent->flag = false;
        return false;
    }

    if (is_left(x)) {
        uncle = x->parent->right_child;
    }
    else {
        uncle = x->parent->left_child;
    }

    expected = false;
    if (!__sync_bool_compare_and_swap(&uncle->flag, expected, true)) {
        dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)uncle);
        dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)x->parent);
        x->parent->flag = false;
        return false;
    }

    dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)uncle);

    // now the process has the flags of x, x's parent and x's uncle
    return true;
}

/**
 * Move up the target node
 */
tree_node *move_inserter_up(tree_node *oldx, vector *local_area)
{
    tree_node *oldp = oldx->parent;
    tree_node *oldgp = oldp->parent;

    bool expected = false;

    tree_node *newx, *newp = NULL, *newgp = NULL, *newuncle = NULL;
    newx = oldgp;
    while (true && newx->parent != NULL) {
        newp = newx->parent;
        expected = false;
        if (!__sync_bool_compare_and_swap(&newp->flag, expected, true)) {
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)newp);
            continue;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)newp);

        newgp = newp->parent;
        if (newgp == NULL)
            break;
        expected = false;
        if (!__sync_bool_compare_and_swap(&newgp->flag, expected, true)) {
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)newgp);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)newp);
            newp->flag = false;
            continue;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)newgp);

        if (newp == newgp->left_child) {
            newuncle = newgp->right_child;
        }
        else {
            newuncle = newgp->left_child;
        }

        expected = false;
        if (!__sync_bool_compare_and_swap(&newuncle->flag, expected, true)) {
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)newuncle);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)newgp);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)newp);
            newgp->flag = false;
            newp->flag = false;
            continue;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)newuncle);

        // now the process has the flags of newp, newgp and newuncle
        break;
    }

    vector_add(local_area, newx);
    vector_add(local_area, newp);
    vector_add(local_area, newgp);
    vector_add(local_area, newuncle);

    return newx;
}

/**
 * find a node by getting flag hand over hand
 * restart when conflict happens
 */
tree_node *par_find(tree_node *root, int value)
{
    bool expect;
    tree_node *root_node;
    tree_node *y;
    tree_node *z;

restart:
    do {
        root_node = root->left_child;
        expect = false;
    } while (!__sync_bool_compare_and_swap(&root_node->flag, expect, true));
    
    y = root_node;
    z = NULL;

    while (!y->is_leaf) {
        z = y; // store old y
        if (value == y->value)
            return y; // find the node y
        else if (value > y->value)
            y = y->right_child;
        else
            y = y->left_child;
        
        expect = false;
        if (!__sync_bool_compare_and_swap(&y->flag, expect, true)) {
            z->flag = false; // release held flag
            msleep(100);
            goto restart;
        }
        if (!y->is_leaf)
            z->flag = false; // release old y's flag
    }
    
    dbg_printf("[WARNING] node with value %d not found.\n", value);
    return NULL; // node not found
}

/**
 * find a node's successor on the left
 * already make sure that the delete node have two non-leaf children
 * by getting flag hand over hand
 * restart when conflict happens
 */
tree_node *par_find_successor(tree_node *delete_node)
{
    bool expect;
    // we already hold the flag of delete_node

    tree_node *y = delete_node->right_child;
    tree_node *z = NULL;

    while (!y->left_child->is_leaf) {
        z = y; // store old y
        y = y->left_child;

        expect = false;
        if (!__sync_bool_compare_and_swap(&y->flag, expect, true)) {
            z->flag = false; // release held flag
            return NULL; // restart outside
        }
        
        z->flag = false; // release old y's flag
    }
    
    return y; // successor found
}