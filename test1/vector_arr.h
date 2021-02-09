#ifndef __VECTOR_ARR_H__
#define __VECTOR_ARR_H__

#define VECTOR_CAPACITY     100

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

#endif
