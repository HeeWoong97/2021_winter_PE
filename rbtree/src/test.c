#include "tree.h"

#include <stdlib.h>

// init

// bool remove_dbg = false;

int main(int argc, char const *argv[])
{
    tree_node *root = rb_init();

    printf("insert 3\n");
    rb_insert(root, 3);
    printf("\ninsert 5\n");
    rb_insert(root, 5);
    printf("\ninsert 15\n");    
    rb_insert(root, 15);
    printf("\ninsert 1\n");
    rb_insert(root, 1);
    printf("\ninsert 2\n");
    rb_insert(root, 2);
    printf("\nshow tree\n");
    show_tree(root);
    printf("\ninsert 7\n");
    rb_insert(root, 7);
    printf("\ninsert 6\n");
    rb_insert(root, 6);
    printf("\nshow tree\n");
    show_tree(root);
    printf("\nremove 2\n");
    rb_remove(root, 2);
    printf("\nshow tree\n");
    show_tree(root);
    printf("\nremove 7\n");
    rb_remove(root, 7);
    printf("\nshow tree\n");
    show_tree(root);
    printf("\nremove 3\n");
    rb_remove(root, 3);
    printf("\nshow tree\n");
    show_tree(root);

    return 0;
}
