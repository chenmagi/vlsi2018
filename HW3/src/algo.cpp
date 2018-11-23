#include "algo.hpp"
#include <algorithm>
#include <boost/foreach.hpp>
#include <math.h>
#include "globalvar.hpp"
#include <iostream>


static bool tiny_module_cmp(tiny_module_t a, tiny_module_t b){
    return a.width > b.width;
}
int build_sorted_module_array(std::vector<module_t> & vec, std::vector<tiny_module_t> &ret_array){
    int i=0;
    for(i=0;i<vec.size();++i){
        tiny_module_t snip;
        snip.id = vec[i].id;
        snip.width = vec[i].shape.w;
        ret_array.push_back(snip);
    }
    sort(ret_array.begin(), ret_array.end(), tiny_module_cmp);
    return 0;
}

int build_b_tree(std::vector<module_t> &vec, std::vector<tiny_module_t> &ordered, boost::shared_ptr<b_node_t> root){
    int sep_idx[5]={0};
    int pop_idx[4]={0};
    int len=ordered.size();
    int step=len/4;
    global_var_t *global_var = global_var_t::get_ref();
    int max_width = global_var->get_die_shape().w;
    int row_width = 0;
    boost::shared_ptr<b_node_t> ptr_current, row_root; 
    //std::cout<<"max width="<<max_width<<std::endl;

    ///< build index array, this will cut ordered-array into 4 parts
    for(int i=0;i<5;++i){
        sep_idx[i]=i*step;
        if(i<4)
            pop_idx[i]=sep_idx[i];
    }
    sep_idx[4]=len;

    ///< setup root node
    pop_idx[0]=1;
    root->module_id = ordered[0].id;
    root->parent = NULL;
    vec[root->module_id].ptr_node = root;
    ptr_current = row_root = root;
    row_width += vec[root->module_id].shape.w;
    len -=1;

    while(len>0){
        for(int i=0;i<4;++i){
            if(pop_idx[i]<sep_idx[i+1]){
                ///< try pop
                int idx=pop_idx[i];  
                unsigned int id = ordered[idx].id;
                
                boost::shared_ptr<b_node_t>child(new b_node_t(id));
                vec[id].ptr_node = child;
                if(row_width + vec[id].shape.w >max_width){
                    //std::cout<<"set new row when append b_node, width="<< row_width + vec[id].shape.w<<std::endl;
                    child->parent = row_root;
                    row_root->rchild = child;
                    row_root = child;
                    row_width=0;    
                }
                else {
                    child->parent = ptr_current;
                    ptr_current->lchild = child;   
                }  
                ptr_current = child;  
                row_width += vec[id].shape.w;
                ///< set pop
                pop_idx[i]+=1;
                len--;
            }
        }
    }

    return 0;
}

shape_t calc_for_die_shape(std::vector<module_t> &vec, double ratio){
    double total_area=0;
    BOOST_FOREACH(auto e, vec){
        total_area+=e.shape.area();
    }    
    total_area*=(1.0+ratio);
    shape_t ret;
    ret.w = ret.h = sqrt(total_area);
    return ret;
}


int build_graphviz(boost::shared_ptr<b_node_t> root, std::ostringstream &ostream, int *node_count){
    if(root==NULL)
        return -1;
    *node_count+=1;    
    if(root->parent==NULL){
        ostream << "digraph G{" <<std::endl;
    }
    if(root->num_of_children()==0){
        ostream<<"sb"<<root->module_id << std::endl;
    }

    if(root->lchild){
        ostream<<"sb"<<root->module_id << " -> ";
        build_graphviz(root->lchild, ostream, node_count);
    } 
    
    if(root->rchild){
        ostream<<"sb"<<root->module_id << " -> ";
        build_graphviz(root->rchild, ostream, node_count);
    }
    
    if(root->parent==NULL){
        ostream << "}" <<std::endl;
    }
    return 0;
}
