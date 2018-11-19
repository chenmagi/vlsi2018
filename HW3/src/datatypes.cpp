#include "datatypes.hpp"
#include <boost/smart_ptr.hpp>
#include <iostream>
shape_t shape_t::build_from_string(const char *str){
    int i=0;
    int len = str!=NULL?strlen(str):0;
    const int num_of_vec=8;
    unsigned int xy[8]={0};
    int idx=0;
    int value=0;
 
    //std::cout<<"shape str="<<str <<std::endl;

    shape_t obj(0,0);
    while(i<len && idx<num_of_vec){
        char ch=str[i++];
        if(ch=='('){ value=0; continue;}
        else if(ch<='9' && ch>='0'){ value*=10; value+=ch-'0'; continue;}
        else if(ch==',' || ch==')'){
            //std::cout<<"idx="<<idx<<", value="<<value<<std::endl;
            xy[idx++]=value; value=0; continue;}
        else if(ch==' ') continue;
        else {
            BOOST_ASSERT_MSG(ch==255, str);
        }
    }
    if(idx==num_of_vec){
        unsigned int h = xy[3]-xy[1];
        unsigned int w = xy[6]-xy[0]; 
        obj.w = w;
        obj.h = h;
    }
    else {
        BOOST_ASSERT_MSG(idx==num_of_vec, "not enough vertex count");
    }
    return obj;    
}

int b_node_t::dfs_visit(boost::shared_ptr<b_node_t> root, int *ret_count){
    if(root==NULL)
        return -1;
    *ret_count+=1;
    
    if(root->lchild){
        //std::cout<<"sb"<<root->module_id << " -> ";
        dfs_visit(root->lchild, ret_count);
    } 
    //else std::cout <<"sb"<<root->module_id << std::endl;
    
    if(root->rchild){
        //std::cout<<"sb"<<root->module_id << " -> ";
        dfs_visit(root->rchild, ret_count);
    }
    //else std::cout <<"sb"<<root->module_id << std::endl;
    return 0;
}
