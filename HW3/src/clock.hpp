#ifndef _CLOCK_HPP_
#define _CLOCK_HPP_
#include <ctime>

struct timer_t {
    
    static timer_t & get_ref(){
        static timer_t inst;
        return inst;
    }

    timer_t & reset(){
        start = clock();
        return *this;
    }
    double elapsed(){
        clock_t now = clock();
        return (double)(now-start)/CLOCKS_PER_SEC;
    }
    private:
    clock_t start;
    static timer_t inst;
    timer_t(){
        start = clock();
    }    
};


#endif