#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "vector_arr.h"

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

void vector_clear(vector *v)
{
    int i;

    for (i = 0; i < vector_total(v); i++) {
        vector_delete(v, i);
    }
}