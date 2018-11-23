#ifndef _ALGO_HPP_
#define _ALGO_HPP_
#include "datatypes.hpp"
#include <sstream>
#include <boost/smart_ptr.hpp>


struct tiny_module_t {
    unsigned int id;
    unsigned int width;

    std::string toString() const{
        std::ostringstream stream;
        stream<<"("<<id<<", "<<width<<")";
        return stream.str();
    }
};

int build_sorted_module_array(std::vector<module_t> & vec, std::vector<tiny_module_t> &ret_array);


int build_b_tree(std::vector<module_t> &vec, std::vector<tiny_module_t> &ordered, 
boost::shared_ptr<b_node_t> root);

int build_graphviz(boost::shared_ptr<b_node_t> root, std::ostringstream &ostream, int *node_count);

shape_t calc_for_die_shape(std::vector<module_t> &vec, double ratio);

#endif