#ifndef __VECTOR_H__
#define __VECTOR_H__

#define VECTOR_INIT_CAPACITY    4

typedef struct vector
{
    void **items;
    int capacity;
    int total;
} vector;

void vector_init(vector *v);
int vector_total(vector *v);
void vector_resize(vector *v, int capacity);
void vector_add(vector *v, void *item);
void vector_set(vector *v, int index, void *item);
void *vector_get(vector *v, int index);
void vector_delete(vector *v, int index);
void vector_clear(vector *v);
void vector_free(vector *v);

#endif