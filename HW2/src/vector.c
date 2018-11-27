#include "vector.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

/**
 * initial a vector and allocate memory for it.
 * @return 0 for success, -1 for error
 */
int vector_setup(vector_t *vector, size_t capacity){
    vector->size=0;
    vector->capacity=capacity;
    vector->data=(unsigned int *)calloc(capacity,sizeof(unsigned int));
    return vector->data==NULL?-1:0;
} 

static inline bool vector_need_grow(vector_t *vector){
    return vector->size==vector->capacity?true:false;
}
static inline int vector_enlarge(vector_t *vector){
    size_t capacity = vector->capacity+DEFAULT_VECTOR_CAPACITY;
    unsigned int *new_data = (unsigned int *)calloc(capacity, sizeof(unsigned int));
    if(new_data==NULL)
        return -1;
    vector->capacity=capacity;
    memcpy(new_data, vector->data, vector->size*sizeof(unsigned int));
    vector->data=new_data;
    return 0;    
}
static inline int vec_elem_compare(const void *a, const void *b){
    return (*(unsigned int *)a) > (*(unsigned int *)b)?1:-1;
}    

int vector_push_back(vector_t *vector, unsigned int elem){
    if(vector_need_grow(vector)){
        int ret=vector_enlarge(vector);
        if(ret!=0) return -1;
    }
    vector->data[vector->size++]=elem;
    return 0;
}
void vector_sort(vector_t *vector){
    qsort((void *)vector->data, vector->size, sizeof(unsigned int), vec_elem_compare); 
} 

unsigned int vector_get(vector_t *vector, int pos){
    assert(pos < vector->size);
    return vector->data[pos];
}

void vector_destory(vector_t *vector){
    vector->size=vector->capacity=0;
    if(vector->data){
        free(vector->data);
        vector->data=NULL;
    }
}

void vector_copy(vector_t *left_op, vector_t *right_op){
    if(!left_op || !right_op)
        return;
    if(left_op->data)
        free(left_op->data);
    if(right_op->data==NULL)
        left_op->data=NULL;
    else {    
        left_op->data=(unsigned int *)malloc(right_op->capacity*sizeof(unsigned int));
        memcpy(left_op->data, right_op->data, sizeof(unsigned int)*right_op->size);
    }
    left_op->size=right_op->size;
    right_op->capacity=right_op->capacity;
    return;
}