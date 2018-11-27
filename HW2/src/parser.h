#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "linkedlist.h"
#include "vector.h"


#define BUF_SZ (4096)
#define MAX_NUM_OF_CELLS (256*1024)
#define MAX_NUM_OF_NETS  (256*1024)


typedef struct parser_t {
	FILE *fp;
	char *buf; 
	size_t buf_alloc; ///< allocated size of buf
	int len; ///< length of content in buf
} parser_t;


enum PART_ID {
    PART_A=0,
	PART_B=1,
	PART_UNKNOWN=2
};

enum LOCK_STATE {
    UNLOCKED=0,
    LOCKED=1,

};

typedef struct cell_info_t {
	unsigned int id; ///< cell's id
	unsigned int size; ///< cell's area
    enum PART_ID part;
	enum LOCK_STATE lock;
	int gain;
    ///< TODO non-copyable data, be careful when using with qsort
    vector_t nets;
}cell_info_t;

#define num_of_pins(ptr) ((cell_info_t *)(ptr)->nets.size)

typedef struct cell_pool_t {
	cell_info_t *array;
	unsigned int total_area; ///< summation of cell's size
	unsigned int max_cell_size;
	unsigned int num_of_cells;
	unsigned int P_max;
	int grp[PART_UNKNOWN];
	size_t capacity; ///< number of slots to store cell_info_t
}cell_pool_t;


typedef struct cell_obj_t {
	cell_info_t *base;
	struct list_head link;
}cell_obj_t;

typedef struct net_t {
	unsigned int id; ///< net's id
	unsigned short int num_of_cells; ///< length of cell_list
	int fn_and_tn[PART_UNKNOWN];
    struct list_head head;	///< list head of cell_obj_t
}net_t;

typedef struct net_pool_t {
	net_t *array;
	unsigned int num_of_nets;
	size_t capacity;
}net_pool_t;

parser_t * parser_create(const char *filename);
cell_pool_t *cellfile_parse(parser_t *parser);
void sort_cell_pool(cell_pool_t *pool);
void free_cell_pool(cell_pool_t *pool);
void parser_destroy(parser_t *parser);


net_pool_t *netfile_parse(parser_t *parser);
void free_net_pool(net_pool_t *pool);
void free_cell_object(cell_info_t *cell);
void dump_cell_pool(const cell_pool_t *cell_list);

int count_P_max(const cell_pool_t *cell_list);
//int update_cell_info(cell_pool_t *cell_list, net_pool_t *net_list);
int verify_pin_count(cell_pool_t *cell_list, net_pool_t *net_list);


#endif
