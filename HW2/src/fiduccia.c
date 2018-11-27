#include "fiduccia.h"
#include <math.h>
#include <assert.h>
#include "log.h"
static int count_cut_size(net_pool_t *net_list);
static void dump_bucket_list(const bucket_list_t *bucket_list);
/**
 * net_id is counting from 1
 */
static void count_fn_and_tn(unsigned int net_id, net_pool_t *net_list,enum PART_ID from, int *fn, int *tn);

/**
 * To allocate bucket array for bucket_list, and setup the P_value of each bucket
 * @return 0 for success, -1 for error
 */
int bucket_list_setup(bucket_list_t *bucket_list, int P_max){
    int i;
    bucket_list->size=P_max*2+1;
    bucket_list->P_max=P_max;
    bucket_list->array=NULL;
    struct bucket *array = (struct bucket *)calloc(bucket_list->size, sizeof(struct bucket));
    if(array==NULL)
        return -1;
    for(i=0;i<bucket_list->size;++i){
       array[i].P_value=P_max-i; ///< decreasing order
       ///< initialize struct list_head
       array[i].head.next=array[i].head.prev=&array[i].head; 
    }
    bucket_list->array = array;
    return 0;
}

static void free_bucket(struct bucket *bucket){
    if(!bucket)
        return;
    while(!list_empty(&bucket->head)){
        bucket_cell_t *entry = list_first_entry(&bucket->head, bucket_cell_t, link);
        list_del(&entry->link);
        free(entry);
    }
}
void bucket_list_destroy(bucket_list_t *bucket_list){
    if(bucket_list && bucket_list->array){
        int i;
        for(i=0;i<bucket_list->size;++i){
            free_bucket(&bucket_list->array[i]);
        }
        free(bucket_list->array);
        bucket_list->array=NULL;
        bucket_list->size=0;
    }
    if(bucket_list) free(bucket_list);
}

static void create_init_partition(cell_pool_t *cell_list, unsigned int *ret_area_A, unsigned int *ret_area_B){
    unsigned int area_A=0, area_B=0, tmp;
    int i;
    unsigned int lower_bound=cell_list->total_area/2;

    cell_list->grp[PART_A]=cell_list->grp[PART_B]=0;
    for(i=0;i<cell_list->num_of_cells;i+=1){
        cell_list->array[i].part=PART_A; area_A+=cell_list->array[i].size;
        cell_list->grp[PART_A]++;
        if(area_A>=lower_bound) break;
        
    }
    for(i=0;i<cell_list->num_of_cells;++i){
        if(cell_list->array[i].part == PART_UNKNOWN){
            cell_list->array[i].part=PART_B; 
            cell_list->grp[PART_B]++;
            area_B+=cell_list->array[i].size;
        }
    }
    for(i=0;i<cell_list->num_of_cells;++i){
        assert(cell_list->array[i].part!=PART_UNKNOWN);
    }
    //debug("total area=%d, area_A=%d, area_B=%d\n", cell_list->total_area, area_A, area_B);
   
    
    *ret_area_A=area_A;
    *ret_area_B=area_B;

    return;
    
}

static bucket_list_t * init_gain(cell_pool_t *cell_list, net_pool_t *net_list, int P_max){
    bucket_list_t *bucket_list = (bucket_list_t *)calloc(1, sizeof(bucket_list_t));
    int i;
    assert(bucket_list!=NULL);
    bucket_list_setup(bucket_list, P_max);
    for(i=0;i<cell_list->num_of_cells;++i){ ///< for each cell i
        enum PART_ID from = cell_list->array[i].part;
        enum PART_ID to = from==PART_A?PART_B:PART_A;
        
        vector_t *nets = &cell_list->array[i].nets;
        int gain=0;
        int idx;
        vector_for_each(idx, nets){ ///< for each net n on Cell i
            unsigned int net_id = vector_get(nets, idx);
            int fn=0;///< number of cells which are in "from" partition and connected with net n
            int tn=0;///< number of cells which are in "to" partition and connected with net n
            count_fn_and_tn(net_id, net_list, from, &fn, &tn);
            net_list->array[net_id-1].fn_and_tn[from]=fn;
            net_list->array[net_id-1].fn_and_tn[to]=tn;
            //printf("cell %d@part %d, net %d, fn=%d, tn=%d\n",
            //cell_list->array[i].id, from, net_id, fn, tn);
            
            if(fn==1) gain+=1;
            if(tn==0) gain-=1;
        }
        bucket_cell_t *bucket_cell = (bucket_cell_t *)calloc(1, sizeof(bucket_cell_t));
        bucket_cell->base=&cell_list->array[i];
        bucket_cell->base->lock=UNLOCKED;
        bucket_cell->base->gain=gain;
        bucket_cell->link.prev=bucket_cell->link.next=&bucket_cell->link;
        assert(gain<=P_max && gain>=-P_max);
        list_add_tail(&bucket_cell->link, &bucket_list->array[P_max-gain].head);
    }
    return bucket_list;

}

static bucket_list_t * backet_list_init_by_gain(cell_pool_t *cell_list,  int P_max){
    int i;
    bucket_list_t *bucket_list = (bucket_list_t *)calloc(1, sizeof(bucket_list_t));
    bucket_list_setup(bucket_list, P_max);
    for(i=0;i<cell_list->num_of_cells;++i){
        
        int gain = cell_list->array[i].gain;
        bucket_cell_t *bucket_cell = (bucket_cell_t *)calloc(1, sizeof(bucket_cell_t));
        bucket_cell->base=&cell_list->array[i];
        bucket_cell->base->lock=UNLOCKED;
        bucket_cell->link.prev=bucket_cell->link.next=&bucket_cell->link;
        //debug("gain=%d, P_max=%d\n",gain,P_max);
        assert(P_max-gain<P_max*2+1);
        list_add_tail(&bucket_cell->link, &bucket_list->array[P_max-gain].head);
    }
    return bucket_list;

}

static void update_backet_list(bucket_list_t *bucket_list, unsigned int cell_id, int prev_gain, int new_gain){
    struct bucket *bucket = &bucket_list->array[bucket_list->P_max-prev_gain];
    struct list_head *pos;
    static int init=0;
    
        
    list_for_each(pos, &bucket->head){
        bucket_cell_t *bucket_cell = list_entry(pos, bucket_cell_t, link);
        if(bucket_cell->base->id!=cell_id)
            continue;
        list_del(pos);
        
        break;
    }   
    
    
    int slot=bucket_list->P_max-new_gain;
    list_add_tail(pos, &bucket_list->array[slot].head);     
    
    
    return;


}

static cell_info_t *get_base_cell(bucket_list_t *bucket_list, unsigned int *area_A, unsigned int *area_B){
    int i;
    int total_area = *area_A+*area_B;
    float criteria=total_area/10.0;
    for(i=0;i<bucket_list->size;i++){
        struct bucket *bucket = &bucket_list->array[i];
        struct list_head *pos;
        if(list_empty(&bucket->head))
            continue;
        list_for_each(pos, &bucket->head){
            bucket_cell_t *bucket_cell = list_entry(pos, bucket_cell_t, link);
            if(bucket_cell->base->lock==LOCKED) continue;
            enum PART_ID from = bucket_cell->base->part;
            enum PART_ID to = from==PART_A?PART_B:PART_A;
            if(from==PART_A){
                float diff =(float) *area_A - (float) *area_B - 2*bucket_cell->base->size;
                if(fabs(diff)>criteria){
                    //debug("[toB]%d %d %d %.2f\n", *area_A, *area_B, bucket_cell->base->size, criteria);
                    //debug("diff=%.2f\n", diff);
                    continue;
                }
                
                *area_A-=bucket_cell->base->size;
                *area_B+=bucket_cell->base->size; 
                
              
            }
            else {
                float diff =(float) *area_A - (float) *area_B - 2*bucket_cell->base->size;
                if(fabs(diff)>criteria){
                    //debug("[toA]%d %d %d %.2f\n", *area_A, *area_B, bucket_cell->base->size, criteria);
                    //debug("diff=%.2f\n",diff);
                    continue;
                }
                
                *area_A+=bucket_cell->base->size;
                *area_B-=bucket_cell->base->size; 
              
            }
            return bucket_cell->base;
        
        }
    }
    return NULL;    


}
/**
 * net_id is counting from 1
 */
static void count_fn_and_tn(unsigned int net_id, net_pool_t *net_list,enum PART_ID from, int *fn, int *tn){
    int idx;
    *fn=*tn=0;
    
    struct list_head *head=&net_list->array[net_id-1].head;
    struct list_head *pos;
    list_for_each(pos, head){ ///< for each cell connected with net n
        cell_obj_t *entry = list_entry(pos, cell_obj_t, link);
        cell_info_t *base = entry->base;
        if(base->part==from) (*fn)++;
        else (*tn)++;
    }
    
    return;            

}

/**
 * @param net_id is starting from 1
 * @parm tn total number of cells which are located in "part" and connected by net_id
 */
static inline void update_critical_net_T(int net_id, int tn, enum PART_ID part, net_pool_t *net_list, bucket_list_t *bucket_list){
    struct list_head *head=&net_list->array[net_id-1].head;
    struct list_head *pos;

    list_for_each(pos, head){ ///< for each cell connected with net net_id
        cell_obj_t *entry = list_entry(pos, cell_obj_t, link);
        cell_info_t *base = entry->base;
        if(base->lock==LOCKED) {
            //printf("[T]cell %d is locked\n", base->id);
            continue; ///< update unlocked cell only
        }
        int offset=(tn==0)?+1:((base->part==part)?-1:0);
        int prev_gain=base->gain;
        //printf("[T]gain update@cell %d, %d->%d\n", base->id, base->gain, base->gain+offset);
        base->gain+=offset;
        assert(base->gain<=bucket_list->P_max && base->gain>=-bucket_list->P_max);
          
        if(offset!=0){
            update_backet_list(bucket_list,base->id, prev_gain, base->gain);
        }
    }
    //dump_bucket_list(bucket_list);
}

/**
 * @param net_id is starting from 1
 * @parm fn total number of cells which are located in "part" and connected by net_id
 */
static inline void update_critical_net_F(int net_id, int fn, enum PART_ID part, net_pool_t *net_list, bucket_list_t *bucket_list){
    struct list_head *head=&net_list->array[net_id-1].head;
    struct list_head *pos;

    list_for_each(pos, head){ ///< for each cell connected with net net_id
        cell_obj_t *entry = list_entry(pos, cell_obj_t, link);
        cell_info_t *base = entry->base;
        if(base->lock==LOCKED) continue; ///< update unlocked cell only
        int offset=(fn==0)?-1:((base->part==part)?+1:0);
        int prev_gain=base->gain;
        //printf("[F]gain update@cell %d, %d->%d\n", base->id, base->gain, base->gain+offset);
        base->gain+=offset;
        assert(base->gain<=bucket_list->P_max && base->gain>=-bucket_list->P_max);
        
        if(offset!=0){
            update_backet_list(bucket_list,base->id, prev_gain, base->gain);
        }
    }
    //dump_bucket_list(bucket_list);
}

static cell_info_t * update_gain(bucket_list_t *bucket_list, net_pool_t *net_list, unsigned int *area_A, unsigned int *area_B, int *ret_gain){
    int i;
    unsigned int total_area=*area_A+*area_B;
    float criteria=total_area/10.0;
    cell_info_t *selected = NULL;
    enum PART_ID from ;
    enum PART_ID to ;
    int idx;
    selected = get_base_cell(bucket_list, area_A, area_B);
    ///< TODO update area size;
    if(selected==NULL)
        return NULL;
    selected->lock=LOCKED; ///< make cell as selected
    
    from=selected->part;
    to=(from==PART_A)?PART_B:PART_A;
    selected->part=to; 
    *ret_gain=selected->gain;
    
    //printf("select c%d, gain=%d\n", selected->id, selected->gain);
    
    vector_for_each(idx, &selected->nets){ ///< for each net n on selected cell
        unsigned int net_id = vector_get(&selected->nets, idx);
        //printf("check net_id %d before move\n", net_id);
        int fn=0;///< number of cells which are in "from" partition and connected with net n
        int tn=0;///< number of cells which are in "to" partition and connected with net n
        ///< check critical net before move
        fn=net_list->array[net_id-1].fn_and_tn[from];
        tn=net_list->array[net_id-1].fn_and_tn[to];
        if(tn==0||tn==1)
            update_critical_net_T(net_id, tn, to, net_list, bucket_list);
        else {
            //printf("net_id %d, tn=%d\n", net_id, tn);
        }    
        
        fn=fn-1;net_list->array[net_id-1].fn_and_tn[from]=fn;
        tn=tn+1;net_list->array[net_id-1].fn_and_tn[to]=tn;
        ///< update critical net after move
        if(fn==0||fn==1)
            update_critical_net_F(net_id, fn, from, net_list, bucket_list); 
    }   
    //printf("select c%d done\n", selected->id);     
     
    return selected; 

}

static void dump_bucket_list(const bucket_list_t *bucket_list){
    int i;
    for(i=0;i<bucket_list->size;++i){
        struct bucket *bucket=&bucket_list->array[i];
        printf("[%d]", bucket->P_value);
        struct list_head *pos;
        list_for_each(pos, &bucket->head){
            bucket_cell_t *bucket_cell = list_entry(pos, bucket_cell_t, link);
            printf("->%d(%c)", bucket_cell->base->id, bucket_cell->base->part==PART_A?'A':'B');
        }
        printf("\n");
    }
}

static int count_cut_size(net_pool_t *net_list){
    int i;
    int cut_size=0;
    struct list_head *pos;
    int pin=0;
    //vector_t cut_set;
    //vector_setup(&cut_set, DEFAULT_VECTOR_CAPACITY);
    for(i=0;i<net_list->num_of_nets;++i){
        int fn=0, tn=0;
        list_for_each(pos, &net_list->array[i].head){ ///< for each cell connected with net n
            cell_obj_t *entry = list_entry(pos, cell_obj_t, link);
            cell_info_t *base = entry->base;
            if(base->part==PART_A) fn++;
            if(base->part!=PART_A) tn++;
            //if(tn>=1&& fn>=1) break;
            pin++;
        }
        if(tn>=1&& fn>=1)
           cut_size++;  
        //vector_push_back(&cut_set, net_list->array[i].id);
    }
    return cut_size;
    /*
    printf("cut_set=");
    for(i=0;i<cut_set.size;++i){
        printf("%d, ", vector_get(&cut_set, i));
    }
    printf("\n");
    */
}

void do_fiduccia(cell_pool_t *cell_list, net_pool_t *net_list, int P_max, int timeout){
    bucket_list_t *bucket_list;
    unsigned int area_A, area_B;
    gain_table_t gain_table;
    cell_info_t *selected;
    int iteration_loop=128;
    int update_loop=1;
    int prev_cut_size=0, cut_size, min_cut_size=65526;
    int i;
    partial_sum_t record[GAIN_TABLE_CAPACITY]={0};
    long long t0=current_timestamp(), t1, t2;
    for(i=0;i<iteration_loop;++i){
        //printf("iteration %d\n", i+1);
        gain_table.size=0;
        gain_table.capacity=GAIN_TABLE_CAPACITY;
        memset(gain_table.history, 0, sizeof(hist_t)*GAIN_TABLE_CAPACITY);
        if(i==0){
            create_init_partition(cell_list, &area_A, &area_B);
            int tmp=count_cut_size(net_list);
            //debug("initial partition, cut size=%d\n",tmp);
            bucket_list=init_gain(cell_list, net_list, P_max);
            //dump_bucket_list(bucket_list);
        }
        else{
            bucket_list_destroy(bucket_list);
            bucket_list=NULL;
            bucket_list= init_gain(cell_list, net_list, P_max);
            //dump_bucket_list(bucket_list);
            //backet_list_init_by_gain(cell_list, P_max); 
        } 
        update_loop=GAIN_TABLE_CAPACITY;
        while(update_loop-->0){
            int ret_gain=0;
            selected=update_gain(bucket_list, net_list, &area_A, &area_B,&ret_gain);
            if(selected==NULL){
                //debug("fail to find base cell @ iteration %d\n", update_loop);
                break;
            }
            else {
                enum PART_ID from=selected->part==PART_A?PART_B:PART_A;
                cell_list->grp[from]-=1;
                cell_list->grp[selected->part]+=1;
            }
            gain_table.history[gain_table.size].cell=selected;
            gain_table.history[gain_table.size++].gain=ret_gain;
        }
        int max_sum=0, idx_k=0,j;
        if(gain_table.size==0)
            continue;
        for(j=0;j<gain_table.size;++j){
            if(j==0) { record[0].idx=j; max_sum=record[0].sum=gain_table.history[j].gain; idx_k=0;}
            else {
                record[j].idx=j;
                record[j].sum=record[j-1].sum+gain_table.history[j].gain;
                if(max_sum<record[j].sum){
                    idx_k=j;
                    max_sum=record[j].sum;
                }
            }
        }
        for(idx_k;idx_k<gain_table.size;idx_k++){
            enum PART_ID pp=gain_table.history[idx_k].cell->part;
            gain_table.history[idx_k].cell->part=pp==PART_A?PART_B:PART_A;
        }
        cut_size=count_cut_size(net_list);
        if(i==0){
            prev_cut_size=cut_size;
            continue;
        }
        else {
            if(abs(prev_cut_size-cut_size)<2)
                break;
            prev_cut_size=cut_size;    
        }
     

    }
    cut_size=count_cut_size(net_list);
    t1=current_timestamp();
    printf("cut_size %d\n", cut_size);
    int grp_count=0;
    for(i=0;i<cell_list->num_of_cells;++i){
        if(cell_list->array[i].part==PART_A) grp_count++;
    }
    printf("A %d\n", grp_count);
    for(i=0;i<cell_list->num_of_cells;++i){
        if(cell_list->array[i].part==PART_A)
            printf("c%d\n", cell_list->array[i].id);
    }
    printf("B %d\n", cell_list->num_of_cells-grp_count);
    for(i=0;i<cell_list->num_of_cells;++i){
        if(cell_list->array[i].part==PART_B)
            printf("c%d\n", cell_list->array[i].id);
    }
    t2=current_timestamp();
    fflush(stdout);
    //printf("io=%lld, computation=%lld\n", t1-t0, t2-t1);
    
    exit(0); ///< force exit
    //update_partition_by_partial_sum(&gain_table);
    //printf("cut size=%d, iteration=%d\n", cut_size, i);
    //printf("area=%d, %d, %.3f\n", area_A, area_B, (fabs)((int)area_A-(int)area_B)/((float)area_A+area_B));
    //dump_bucket_list(bucket_list);
    bucket_list_destroy(bucket_list);
}
