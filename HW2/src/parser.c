#include "parser.h"
#include "log.h"
#include "vector.h"

static cell_pool_t g_cell_pool;
static net_pool_t g_net_map;


parser_t * parser_create(const char *filename){
    parser_t *parser = (parser_t *)malloc(sizeof(parser_t));
    if(!parser)
        return NULL;
    
    parser->buf = (char *)malloc(BUF_SZ);
    if(!parser->buf){
        free(parser);
        return NULL;
    }
    parser->fp = fopen(filename, "r");
    parser->buf_alloc=BUF_SZ;
    parser->len=0;



    return parser;

}
void parser_destroy(parser_t *parser){
    if(!parser)
        return;
    
    if(parser->fp)
        fclose(parser->fp);
    free(parser->buf);    
    free(parser);
    return;    
}

static size_t parser_read(parser_t *parser){
    size_t len = fread(parser->buf,1, parser->buf_alloc,  parser->fp);
    //debug("parser_read: buf addr=%u, buf_alloc=%d, fp=%u, result=%d\n", 
    //    parser->buf, parser->buf_alloc, parser->fp, len);
    parser->len=len;
    return len;
}




static inline void push2pool(cell_pool_t *pool, unsigned int id, unsigned int value){
    
    //debug("push cell %d: %d\n", id, value);
    if(pool->num_of_cells >= pool->capacity){
        ///< TODO extend pool buffer to accept more data
        assert(pool->num_of_cells<pool->capacity);
    }
    ((cell_info_t *)&pool->array[pool->num_of_cells])->id=id;
    ((cell_info_t *)&pool->array[pool->num_of_cells])->size=value;
    ((cell_info_t *)&pool->array[pool->num_of_cells])->part=PART_UNKNOWN;
    ((cell_info_t *)&pool->array[pool->num_of_cells])->gain=0;
    pool->num_of_cells++;
    return;
}
static inline int cell_compare(const void *a, const void *b){
    assert(((cell_info_t *)a)->id != ((cell_info_t *)b)->id);
    return ((cell_info_t *)a)->id > ((cell_info_t *)b)->id?1:-1;
}

void sort_cell_pool(cell_pool_t *pool){
    int i;
    qsort((void *)pool->array, pool->num_of_cells, sizeof(cell_info_t), cell_compare);
    //for(i=0;i<pool->num_of_cells;++i){
    //    printf("%06d: %d, %d\n", i, pool->array[i].id, pool->array[i].size);
    //}
}

void free_cell_pool(cell_pool_t *pool){
    if(!pool)
      return;
    if(pool->array){
        free(pool->array);
    }
    pool->array=NULL;
    pool->num_of_cells=0;
    pool->capacity=0;
}
cell_pool_t *cellfile_parse(parser_t *parser){
    ///< FIXME segmentation fault when file is empty
    int len; 
    int i, id, value;
    int count=0;
    char ch;
    unsigned int total_area=0;
    unsigned int max_cell_size=0;
    enum OP {
      ID=0,
      VALUE=1,
      NONE=2
    } op = NONE;
    
    g_cell_pool.num_of_cells=0;
    g_cell_pool.capacity = MAX_NUM_OF_CELLS;
    g_cell_pool.array = (cell_info_t *)calloc(MAX_NUM_OF_CELLS, sizeof(struct cell_info_t));
    ///<TODO check whether allocation is successful or not
    
    id=-1;
    value=0;
    while((len=parser_read(parser))!=0){
        count+=len;
        for(i=0;i<parser->len;++i){
          ch = parser->buf[i];
          switch(op){
            case ID:
                if(ch==' ') { op = VALUE; break;}
                id*=10; id+=ch-'0'; break;
            case VALUE:
                if(ch==0x0a || ch==' ') { op=NONE; 
                    vector_t *vec=&g_cell_pool.array[g_cell_pool.num_of_cells].nets;
                    vector_setup(vec, DEFAULT_VECTOR_CAPACITY);
                    push2pool(&g_cell_pool, id, value); 
                    total_area += value;
                    max_cell_size=max_cell_size>value?max_cell_size:value;
                    break;
                }
                value*=10; value+=ch-'0'; break;
            case NONE:
                if(ch=='c') { op=ID; id=0; value=0; break;}
          }
        }
    }    
    g_cell_pool.total_area=total_area;
    g_cell_pool.max_cell_size=max_cell_size;
   // debug("%s: total area = %d, max cell size=%d\n", __FUNCTION__, total_area, max_cell_size);
    return &g_cell_pool;

}

static inline net_t *get_slot_and_incr(net_pool_t *map){
    assert(map->num_of_nets<map->capacity);
    return &map->array[map->num_of_nets++];
}

static int cell_id_cmp(const void *a, const void *b){
    unsigned int cell_id = *(unsigned int *)a;
    cell_info_t *cell_ptr = (cell_info_t *)b;
    if(cell_id==cell_ptr->id)
        return 0;
    else return cell_id > cell_ptr->id?1:-1;
}

static inline void add_cell_info_to_net(net_t *ptr, unsigned int cell_id){
    cell_obj_t *obj=(cell_obj_t *)malloc(sizeof(cell_obj_t));
    cell_info_t *base = NULL;
    base = (cell_info_t *)bsearch(&cell_id, g_cell_pool.array, 
                                              g_cell_pool.num_of_cells, sizeof(cell_info_t), cell_id_cmp);
    assert(base!=NULL);
    obj->base = base;
    list_add_tail(&obj->link, &ptr->head );
    vector_push_back(&base->nets, ptr->id);
    ptr->num_of_cells+=1;
}

static inline void dump_net(net_t *ptr){
    printf("NET n%d { ",ptr->id);
    struct list_head *pos;
    list_for_each(pos,&ptr->head){
        cell_obj_t *obj = container_of(pos, cell_obj_t, link);
        printf("c%d ", obj->base->id);
    }
    printf("}\n");
}

net_pool_t *netfile_parse(parser_t *parser){
    int len; 
    int i, net_id, cell_id;
    int count=0;
    char ch;
    net_t *ptr=NULL;
    enum OP {
      HDR=0,
      N_ID=1,
      L_BR=2,
      C_ID=3,
      //R_BR=4,
      NONE=5
    } op = NONE;
    
    g_net_map.num_of_nets=0;
    g_net_map.capacity = MAX_NUM_OF_NETS;
    g_net_map.array = (net_t *)calloc(MAX_NUM_OF_NETS*sizeof(struct net_t), 1);
    

    net_id=0;
    cell_id=0;
    int close=0;
    while((len=parser_read(parser))!=0){
        count+=len;
        for(i=0;i<parser->len;++i){
          ch = parser->buf[i];
          switch(op){
            case N_ID:
                if(ch==' ') { op = L_BR; ptr->id=net_id; break;}
                net_id*=10; net_id+=ch-'0'; break;
            
            case HDR:
                if(ch=='E'||ch=='T'||ch==' '){break;}
                if(ch=='n'){op=N_ID; net_id=0;  break;}
            case L_BR:
                if(ch=='{'||ch==' '){if(ch=='{') close=0;break;}
                if(ch=='c'){op=C_ID; cell_id=0; break;}
            case C_ID:
                if(ch==' ') { add_cell_info_to_net(ptr,cell_id); break;}
                if(ch=='c') {cell_id=0; break;}
                if(ch==0x0a) {
                   if(close==1) op=NONE; 
                    //dump_net(ptr);
                    break;
                }
                else if(ch=='}'){ op=NONE; close=1; break;}
                cell_id*=10; cell_id+=ch-'0'; break;

            case NONE:
                if(ch=='N') { op=HDR;  ptr=get_slot_and_incr(&g_net_map);
                    ptr->num_of_cells=0;
                    ptr->head.next=ptr->head.prev=&ptr->head;
                break;}
          }
        }
    }    
    //debug("%s: total read bytes = %d\n", __FUNCTION__, count);
    return &g_net_map;

}


void dump_cell_pool(const cell_pool_t *cell_list){
    int i;
    for(i=0;i<cell_list->num_of_cells;++i){
        cell_info_t *cell_ptr = &cell_list->array[i];
        int pos;
        printf("cell %d: lock=%d, part=%d\n\t ", cell_ptr->id,cell_ptr->lock, cell_ptr->part);
        vector_for_each(pos,&cell_ptr->nets){
            printf("%d ",vector_get(&cell_ptr->nets, pos));
        }
        printf("\n");
    }
}
#if (0)
/**
 * This function will update connected nets into each cell_info_t in cell_list
 */
int update_cell_info(cell_pool_t *cell_list, net_pool_t *net_list){
    int i;
    for(i=0;i<net_list->num_of_nets;++i){
        net_t *net_ptr = &g_net_map.array[i];
        struct list_head *pos;
        list_for_each(pos, &net_ptr->head){
            unsigned int cell_id= ((cell_id_t *)container_of(pos,cell_id_t, link))->id;
            unsigned int net_id = net_ptr->id;
            cell_info_t *matched=(cell_info_t *)bsearch(&cell_id, g_cell_pool.array, 
                                              g_cell_pool.num_of_cells, sizeof(cell_info_t), cell_id_cmp);
            assert(matched!=NULL);
            vector_push_back(&matched->nets, net_id);
            //debug("add net %d to cell %d\n",cell_id, net_id );
        }
    }

    //dump_cell_pool(&g_cell_pool);

    return 0;
}
#endif

void free_net_pool(net_pool_t *pool){
    ///< TODO implement this
}


static int count_pins_from_cell_list(cell_pool_t *cell_list){
    int i;
    int sum =0 ;
    for(i=0;i<cell_list->num_of_cells;++i){
        sum+=cell_list->array[i].nets.size;
    }
    return sum;

}

static int count_pins_from_net_list(net_pool_t *net_list){
    int i;
    int sum=0;
    for(i=0;i<net_list->num_of_nets;++i){
        sum+=net_list->array[i].num_of_cells;
    }
    return sum;
}
int count_P_max(const cell_pool_t *cell_list){
    int i;
    int max =0 ;
    for(i=0;i<cell_list->num_of_cells;++i){
        max=max>cell_list->array[i].nets.size?max:cell_list->array[i].nets.size;
    }
    return max;
}
/**
 * Count pin numbers from cell_list and net_list as well.
 * Compare these two numbers. 
 * @return -1 for comparison result is not equal, 0 for two pin numbers is equal.
 */
int verify_pin_count(cell_pool_t *cell_list, net_pool_t *net_list){

    int num_of_pin0, num_of_pin1;
    int i;
    num_of_pin0=count_pins_from_cell_list(cell_list);
    num_of_pin1=count_pins_from_net_list(net_list);
    printf("%d-->%d\n", num_of_pin0, num_of_pin1);
    return num_of_pin0==num_of_pin1?0:-1;
}


void free_cell_object(cell_info_t *cell){
    if(!cell)
        return;
    vector_destory(&cell->nets);
    free(cell);
}

#include <sys/time.h>

long long current_timestamp(void) {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}
