#include <stdio.h>
#include "parser.h"
#include "linkedlist.h"
#include "log.h"
#include <assert.h>
#include "fiduccia.h"

int main(int argc, char **argv){
    parser_t *parser;
    cell_pool_t *cell_pool = NULL;
    net_pool_t *net_map = NULL;
    
    if(argc>=2){
        parser = parser_create(argv[1]);
        assert(parser!=NULL);
        cell_pool = cellfile_parse(parser);
        sort_cell_pool(cell_pool);
        parser_destroy(parser);
    }
    

    if(argc>=3){
        parser = parser_create(argv[2]);
        net_map = netfile_parse(parser);
        parser_destroy(parser);
        //update_cell_info(cell_pool, net_map);
    }
    int P_max = count_P_max(cell_pool);
    //debug("P_max=%d\n", P_max);
    do_fiduccia(cell_pool, net_map, P_max,-1);
    //verify_pin_count(cell_pool, net_map);
    //dump_cell_pool(cell_pool);
    free_cell_pool(cell_pool);

    return 0;

}
  
