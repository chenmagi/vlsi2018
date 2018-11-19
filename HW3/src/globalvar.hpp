#ifndef _GLOBALVAR_HPP_
#define _GLOBALVAR_HPP_
#include "datatypes.hpp"
#include <iostream>

enum placement_t {
    PLACEMENT_HARD=0,
    PLACEMENT_SOFT=1,
    PLACEMENT_UNKNOWN=2
};

struct global_var_t {
    
    static global_var_t * get_ref(void){
        static global_var_t obj(PLACEMENT_HARD);
        return &obj;
    }
    void set_placement(placement_t mode){
        placement = mode;
    }
    placement_t get_placement(){
        return placement;
    }
    void set_white_ratio(double ratio){
        white_space_ratio = ratio;
    }
    double get_white_ratio(){
        return white_space_ratio;
    }
    void set_die_shape(shape_t shape){
        die_shape = shape;
    }
    shape_t get_die_shape(){
        return die_shape;
    }
    private:
    global_var_t(placement_t mode){
        placement = mode;
        white_space_ratio=0.1;
        
    }
    placement_t placement;
    double white_space_ratio;
    shape_t die_shape; 
    

};



#endif