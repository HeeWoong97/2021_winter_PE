#include "tree.h"
#include "vector_arr.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>

/******************
 * helper function
 ******************/

vector nodes_own_flag[16];
int cnt;

/**
 * clear local area
 */
void clear_local_area(long thread_index)
{   
    int i;

    if (vector_total(&nodes_own_flag[thread_index]) == 0) return;
    dbg_printf("[Flag] Clear\n");
    for (i = 0; i < vector_total(&nodes_own_flag[thread_index]); i++) {
        tree_node *node = vector_get(&nodes_own_flag[thread_index], i);
        node->flag = false;
        dbg_printf("[Flag]      %d, 0x%lx, %d\n",
                node->value, (unsigned long)node, (int)node->flag);
    }
    vector_clear(&nodes_own_flag[thread_index]);
}

/**
 * true if the node is in our local area
 */
bool is_in_local_area(tree_node *target_node, long thread_index)
{
    int i;

    for (i = 0; i < vector_total(&nodes_own_flag[thread_index]); i++) {
        tree_node *node = vector_get(&nodes_own_flag[thread_index], i);
        if (node == target_node) return true;        
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
bool get_markers_above(tree_node *start, tree_node *z, bool release, long thread_index)
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
bool setup_local_area_for_delete(tree_node *y, tree_node *z, long thread_index)
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
    if (!get_markers_above(yp, z, true, thread_index)) {
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
    vector_init(&nodes_own_flag[thread_index]);
    vector_add(&nodes_own_flag[thread_index], x);
    vector_add(&nodes_own_flag[thread_index], w);
    vector_add(&nodes_own_flag[thread_index], yp);
    if (!w->is_leaf) {
        vector_add(&nodes_own_flag[thread_index], wlc);
        vector_add(&nodes_own_flag[thread_index], wrc);
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
bool get_flags_and_markers_above(tree_node *start, int numAdditional, long thread_index)
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
    if (!get_markers_above(start, NULL, false, thread_index))
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
bool release_markers_above(tree_node *start, tree_node *z, long thread_index)
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
tree_node *move_deleter_up(tree_node *oldx, long thread_index)
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
    while (!get_flags_and_markers_above(oldp, 1, thread_index))
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
    vector_clear(&nodes_own_flag[thread_index]);
    vector_add(&nodes_own_flag[thread_index], newx);
    vector_add(&nodes_own_flag[thread_index], neww);
    vector_add(&nodes_own_flag[thread_index], newp);
    vector_add(&nodes_own_flag[thread_index], newwlc);
    vector_add(&nodes_own_flag[thread_index], newwrc);
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
void fix_up_case1(tree_node *x, tree_node *w, long thread_index)
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
    vector_clear(&nodes_own_flag[thread_index]);
    vector_add(&nodes_own_flag[thread_index], x);
    vector_add(&nodes_own_flag[thread_index], x->parent);
    vector_add(&nodes_own_flag[thread_index], w);
    vector_add(&nodes_own_flag[thread_index], w->left_child);
    vector_add(&nodes_own_flag[thread_index], w->right_child);
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
void fix_up_case3(tree_node *x, tree_node *w, long thread_index)
{
    int i;
    tree_node *oldw = w->right_child;
    tree_node *oldwrc = oldw->right_child;

    // clear all the markers within old local area
    for (i = 0; i < vector_total(&nodes_own_flag[thread_index]); i++) {
        tree_node *node = vector_get(&nodes_own_flag[thread_index], i);
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
    vector_clear(&nodes_own_flag[thread_index]);
    vector_add(&nodes_own_flag[thread_index], x);
    vector_add(&nodes_own_flag[thread_index], x->parent);
    vector_add(&nodes_own_flag[thread_index], w);
    vector_add(&nodes_own_flag[thread_index], w->left_child);
    vector_add(&nodes_own_flag[thread_index], oldw);
}

/**
 * fix the side effect of delete case 1 - mirror case
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation 
 *    --> clear all the 'naughty' markers within old local area
 */
void fix_up_case1_r(tree_node *x, tree_node *w, long thread_index)
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
    vector_clear(&nodes_own_flag[thread_index]);
    vector_add(&nodes_own_flag[thread_index], x);
    vector_add(&nodes_own_flag[thread_index], x->parent);
    vector_add(&nodes_own_flag[thread_index], w);
    vector_add(&nodes_own_flag[thread_index], w->left_child);
    vector_add(&nodes_own_flag[thread_index], w->right_child);
}

/**
 * fix the side effect of delete case 3 - mirror case
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation
 */
void fix_up_case3_r(tree_node *x, tree_node *w, long thread_index)
{
    int i;
    tree_node *oldw = w->left_child;
    tree_node *oldwlc = oldw->left_child;

    // clear all the markers within old local area
    for (i = 0; i < vector_total(&nodes_own_flag[thread_index]); i++) {
        tree_node *node = vector_get(&nodes_own_flag[thread_index], i);
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
    vector_clear(&nodes_own_flag[thread_index]);
    vector_add(&nodes_own_flag[thread_index], x);
    vector_add(&nodes_own_flag[thread_index], x->parent);
    vector_add(&nodes_own_flag[thread_index], w);
    vector_add(&nodes_own_flag[thread_index], oldw);
    vector_add(&nodes_own_flag[thread_index], w->right_child);
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

	// printk("vector_add 1 (vector size: %d, cnt: %d)", vector_total(local_area), cnt);
    vector_add(local_area, newx);
	// printk("vector_add 2 (vector size: %d, cnt: %d)", vector_total(local_area), cnt);
    vector_add(local_area, newp);
	// printk("vector_add 3 (vector_size: %d, cnt: %d)", vector_total(local_area), cnt);
    vector_add(local_area, newgp);
	// printk("vector_add 4 (vector_size: %d, cnt: %d)", vector_total(local_area), cnt);
    vector_add(local_area, newuncle);
	// printk("move_inserter_up finished (cnt: %d)\n", cnt);

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
