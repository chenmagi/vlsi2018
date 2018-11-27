#ifndef _VECTOR_H_
#define _VECTOR_H_
#include <stddef.h>
#include <stdbool.h>
#define DEFAULT_VECTOR_CAPACITY (32)
/**
 * This implementation is only for integer datatype.
 * 
 */
typedef struct vector_t {
	size_t size;
	size_t capacity;
	unsigned int *data;
}vector_t;

int vector_setup(vector_t *vector, size_t capacity);
int vector_push_back(vector_t *vector, unsigned int elem);
void vector_sort(vector_t *vector); 
void vector_copy(vector_t *left_op, vector_t *right_op);
unsigned int vector_get(vector_t *vector, int pos);
void vector_destory(vector_t *vector);



#define vector_for_each(pos, vec) \
	for (pos = 0; pos < (vec)->size; pos ++)







#endif
