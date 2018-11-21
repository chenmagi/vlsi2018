#include "datatypes.hpp"
#include <boost/smart_ptr.hpp>
#include <iostream>
#include <queue>
#include "globalvar.hpp"
#include <algorithm>
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

static void init_contour(std::vector<unsigned int>&contour, int max_width){
    int i;
    contour.clear();
    for(i=0;i<max_width;++i){
        contour.push_back(0);
    }
}

static void update_horz_contour(std::vector<unsigned int> &contour, coordinate_t &origin, shape_t &shape){
    int x = origin.x;
    for(x = origin.x; x<origin.x+shape.w;++x){
        contour[x]+=shape.h;
    }
    return;
}

static void update_vert_contour(std::vector<unsigned int> &contour, coordinate_t &origin, shape_t &shape){
    int y = origin.y;
    for(y = origin.y; y<origin.y+shape.h;++y){
        contour[y]+=shape.w;
    }
    return;
}

static void place_left_child(std::vector<unsigned int> &horz_contour,std::vector<unsigned int> &vert_contour,
module_t & parent, module_t &lchild){
    int x = parent.origin.x + parent.shape.w;
    int y = horz_contour[x];
    lchild.origin.set(x,y);
    update_horz_contour(horz_contour, lchild.origin, lchild.shape);
    update_vert_contour(vert_contour, lchild.origin, lchild.shape);
}

static void place_right_child(std::vector<unsigned int> &horz_contour,std::vector<unsigned int> &vert_contour,
module_t & parent, module_t &rchild){
    int x = parent.origin.x;
    int y = horz_contour[x];
    rchild.origin.set(x,y);
    update_horz_contour(horz_contour, rchild.origin, rchild.shape);
    update_vert_contour(vert_contour, rchild.origin, rchild.shape);
}

shape_t b_node_t::pack(boost::shared_ptr<b_node_t>root,std::vector<module_t> & module_array, std::vector<unsigned int> &horz_contour,std::vector<unsigned int>&vert_contour){
    std::queue<unsigned int> bfs_queue;
    global_var_t *global_var = global_var_t::get_ref();

    init_contour(horz_contour, global_var->get_die_shape().w*2);
    init_contour(vert_contour, global_var->get_die_shape().w*2);

    bfs_queue.push(root->module_id);
    ///< place root node
    module_array[root->module_id].origin.set(0,0);
    ///< update contour
    update_horz_contour(horz_contour,module_array[root->module_id].origin, module_array[root->module_id].shape);
    update_vert_contour(vert_contour,module_array[root->module_id].origin, module_array[root->module_id].shape);


    while(!bfs_queue.empty()){
        unsigned int poped_id = bfs_queue.front();
        bfs_queue.pop();
        boost::shared_ptr<b_node_t> sub_root = module_array[poped_id].ptr_node;
        boost::shared_ptr<b_node_t> lchild = sub_root->lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root->rchild;
        if(lchild!=NULL){
            unsigned int child_id = lchild->module_id;
            place_left_child(horz_contour, vert_contour, module_array[poped_id], module_array[child_id]);
            bfs_queue.push(child_id);

        }
        if(rchild!=NULL){
            unsigned int child_id = rchild->module_id;
            place_right_child(horz_contour, vert_contour, module_array[poped_id], module_array[child_id]);
            bfs_queue.push(child_id);
        }
    }
    shape_t result;
    result.h = *std::max_element(horz_contour.begin(), horz_contour.end());
    result.w = *std::max_element(vert_contour.begin(), vert_contour.end());

    return result;
}
static bool contains(unsigned int rect0[4][2], unsigned int rect1[4][2]){
    for(int i=0;i<4;++i){
        int x=rect0[i][0];
        int y=rect1[i][1];
        if(x>rect1[0][0] && x<rect1[2][0])
            if(y>rect1[0][1] && y < rect1[2][1])
                return true;
    }
    return false;
}
bool b_node_t::verify(boost::shared_ptr<b_node_t>root,std::vector<module_t> & module_array){
    std::queue<unsigned int> bfs_queue;
    
    bfs_queue.push(root->module_id);
    int overlap_count=0;
    
    while(!bfs_queue.empty()){
        unsigned int poped_id = bfs_queue.front();
        bfs_queue.pop();
        unsigned int rect0[4][2] = {0};
        module_array[poped_id].get_rect(rect0);
        for(int i=0;i<module_array.size();++i){
            if(i==poped_id) continue;
            
            unsigned int rect1[4][2]={0};
            module_array[i].get_rect(rect1);
            if(contains(rect0, rect1)){
                std::cout << "module "<<poped_id<<" is contained by module "<<i <<std::endl;
                overlap_count++;
            }
            else if(contains(rect1, rect0)){
                std::cout << "module "<<i<<" is contained by module "<<poped_id <<std::endl;
                overlap_count++;
            }
        }

        boost::shared_ptr<b_node_t> sub_root = module_array[poped_id].ptr_node;
        boost::shared_ptr<b_node_t> lchild = sub_root->lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root->rchild;
        if(lchild!=NULL){
            unsigned int child_id = lchild->module_id;
            bfs_queue.push(child_id);

        }
        if(rchild!=NULL){
            unsigned int child_id = rchild->module_id;
            bfs_queue.push(child_id);
        }
    }    
    return overlap_count==0?false:true;

}

