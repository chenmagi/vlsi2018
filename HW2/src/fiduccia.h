#ifndef _FIDUCCIA_H_
#define _FIDUCCIA_H_

#include "parser.h"
#include "linkedlist.h"
#include <time.h>
struct bucket {
    int P_value;
    struct list_head head; ///< head for bucket_cell_t list
};


typedef struct bucket_cell_t {    
    cell_info_t *base;
    struct list_head link;
}bucket_cell_t;



typedef struct bucket_list_t {
    size_t size;
    int P_max;
    struct bucket *array;
}bucket_list_t;

typedef struct hist_t {
    cell_info_t *cell;
    int gain;
}hist_t;

typedef struct partial_sum_t {
    int idx;
    int sum;
}partial_sum_t;

#define GAIN_TABLE_CAPACITY (4096)

typedef struct gain_table_t {
    size_t capacity;
    size_t size;
    hist_t history[GAIN_TABLE_CAPACITY];
}gain_table_t;


int bucket_list_setup(bucket_list_t *bucket_list, int P_max);
void bucket_list_destroy(bucket_list_t *bucket_list);

void do_fiduccia(cell_pool_t *cell_list, net_pool_t *net_list, int P_max,int timeout);


#endif