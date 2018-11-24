#ifndef _CLOCK_HPP_
#define _CLOCK_HPP_
#include <ctime>

struct simple_timer_t {
    
    static simple_timer_t & get_ref(){
        static simple_timer_t inst;
        return inst;
    }

    simple_timer_t & reset(){
        start = clock();
        return *this;
    }
    double elapsed(){
        clock_t now = clock();
        return (double)(now-start)/CLOCKS_PER_SEC;
    }
    private:
    clock_t start;
    simple_timer_t(){
        start = clock();
    }    
};


#endif