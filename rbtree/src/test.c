#include "tree.h"

#include <stdlib.h>

// init

int main(int argc, char const *argv[])
{
    tree_node *root = rb_init();

    rb_insert(root, 3);
    printf("insert 3\n");
    rb_insert(root, 5);
    printf("insert 5\n");
    rb_insert(root, 15);
    printf("insert 15\n");
    rb_insert(root, 1);
    printf("insert 1\n");
    rb_insert(root, 2);
    printf("insert 2\n");
    // show_tree(root);
    rb_insert(root, 7);
    printf("insert 7\n");
    rb_insert(root, 6);
    printf("insert 6\n");
    // show_tree(root);
    rb_remove(root, 2);
    printf("remove 2\n");
    // show_tree(root);
    rb_remove(root, 7);
    printf("remove 7\n");
    // show_tree(root);
    rb_remove(root, 3);
    printf("remove 3\n");
    // show_tree(root);
    return 0;
}
