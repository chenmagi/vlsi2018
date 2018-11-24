#ifndef _SIMULATED_ANNEALING_HPP_
#define _SIMULATED_ANNEALING_HPP_
#include "clock.hpp"
#include <stdlib.h>
#include <math.h>
#include "datatypes.hpp"
#include <boost/smart_ptr.hpp>
#include <limits>
#include <vector>
#include <sstream>

struct random_t {
    enum dice_t {
        FIRST_DEVIATION = 1,
        SECOND_DEVIATION = 2,
        THIRD_DEVIATION = 3
    };
    
    static unsigned seed;
    
    static random_t &get_ref(){
        static random_t inst;
        return inst;
    }

    static void init(unsigned _seed){
        seed=_seed;
        srand(_seed);
    }

    void reseed(unsigned int _seed){
        seed=_seed;
        srand(_seed);
    }
    
    dice_t rolling(){
        double u = rand() / (double)RAND_MAX;
        double v = rand() / (double)RAND_MAX;
        double x = sqrt(-2 * log(u)) * cos(2 * M_PI * v) * deviation + mean;
        if(x>mean-deviation && x<mean+deviation)
            return FIRST_DEVIATION;
        else if(x>mean-2*deviation && x<mean+2*deviation)
            return SECOND_DEVIATION;
        return THIRD_DEVIATION;    
    }
    



    private:
    
    static  double deviation;
    static  double mean;
    
    random_t(){
    }
    random_t(const random_t &);
    random_t & operator=(const random_t&);
    

    
    
    
};


struct solution_t {
    boost::shared_ptr<b_node_t> tree_root;
    std::vector<boost::shared_ptr<b_node_t> > lookup_tbl;
    double cost;
    shape_t die_shape;
    static  double alpha;
    solution_t(){
        tree_root.reset();
        cost = std::numeric_limits<unsigned long int>::max();
    }

    void build_from_b_tree(boost::shared_ptr<b_node_t> src_root, int num_of_nodes);

    void update_cost(std::vector<module_t> &module_array, std::vector<net_t> &net_array, 
    std::vector<terminal_t> &pin_array);    


    inline solution_t & operator=(const solution_t &other){
        build_from_b_tree(other.tree_root, other.lookup_tbl.size());
        cost = other.cost;
        die_shape = other.die_shape;
        return *this;
    }


    std::string toString() const{
        std::ostringstream stream;
        stream<<"cost="<<cost<<", ";
        stream<<"die_shape="<<die_shape.w <<" x " << die_shape.h ;
        return stream.str();
    }
};

struct simulated_annealing_t {
    static  double tc_start ;
    static  double tc_end ;
    static  double cooling_factor ;
    solution_t best_sol;
    solution_t cur_sol;
    
    
    void run(std::vector<module_t> & module_array, std::vector<net_t> &net_array, std::vector<terminal_t> &pin_array,int timeout);

    private:
    double acceptance(double new_cost, double old_cost, double tc){
        if(new_cost < old_cost)
            return 1.0;
        else return exp((old_cost - new_cost)/tc);
    }
    ///< TODO copy solution is an overhead
    solution_t get_next_solution(std::vector<module_t> & module_array, solution_t &cur_sol);
};


#endif