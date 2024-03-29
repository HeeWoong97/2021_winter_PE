#include "tree.h"

#include <stdlib.h>

extern __thread long thread_index;

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
    if (node->is_leaf) {
        fprintf(stderr, "[ERROR] invalid rotate on NULL node.\n");
        exit(1);
    }
    
    if (node->right_child->is_leaf) {
        fprintf(stderr, 
                "[ERROR] invalid rotate on node with NULL right child.\n");
        exit(1);
    }

    tree_node *right_child = node->right_child;
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
    if (node->is_leaf) {
        fprintf(stderr, "[ERROR] invalid rotate on NULL node.\n");
        exit(1);
    }

    if (node->left_child->is_leaf) {
        fprintf(stderr,
                "[ERROR] invalid rotate on node with NULL left child.\n");
        exit(1);
    }

    tree_node *left_child = node->left_child;
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

    restart:;

    tree_node *z = NULL;
    tree_node *curr_node = root->left_child;
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
        free(z->left_child);
        z->left_child = new_node;
    }
    else {
        free(z->right_child);
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
    // init thread local nodes with flag
    clear_local_area();

    tree_node *new_node;
    new_node = (tree_node *)malloc(sizeof(tree_node));
    new_node->color = RED;
    new_node->value = value;
    new_node->left_child = create_leaf_node();
    new_node->right_child = create_leaf_node();
    new_node->is_leaf = false;
    new_node->parent = NULL;
    new_node->flag = false;
    new_node->marker = DEFAULT_MARKER;

    tree_insert(root, new_node); // normal insert

    tree_node *curr_node = new_node;
    
    tree_node *parent, *uncle = NULL, *grandparent = NULL;

    parent = curr_node->parent;
    vector local_area;
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
        for (int i = 0; i < vector_total(&local_area); i++) {
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
    for (int i = 0; i < vector_total(&local_area); i++) {
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
    dbg_printf("[Remove] thread %ld value %d\n", thread_index, value);
    // init thread local nodes with flag
    clear_local_area();
restart:;

    tree_node *z = par_find(root, value);
    tree_node *y; // actual delete node
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
    tree_node *replace_node = replace_parent(root, y);

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
