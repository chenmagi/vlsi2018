#include "datatypes.hpp"
#include <boost/smart_ptr.hpp>
#include <iostream>
#include <queue>
#include "globalvar.hpp"
#include <algorithm>
#include <vector>

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

    //if(root->num_of_children()==0){
    //    std::cout<<"sb"<<root->module_id << std::endl;
    //}
    
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
static int max_contour(std::vector<unsigned int> &contour, int start, int end){
    int max=contour[start];
    for(int i=start+1;i<end;++i){
        if(max<contour[i]) max=contour[i];
    }
    return max;
}
static void place_left_child2(std::vector<unsigned int> &horz_contour,std::vector<unsigned int> &vert_contour,
module_t & parent,bool parent_rotated, module_t &lchild, bool child_rotated){
    int x = parent.origin.x + (parent_rotated?parent.shape.h:parent.shape.w);
    int y = max_contour(horz_contour, x, x+(child_rotated?lchild.shape.h:lchild.shape.w));
    lchild.origin.set(x,y);
    shape_t rotated_shape(lchild.shape.h, lchild.shape.w);
    if(child_rotated){
        update_horz_contour(horz_contour, lchild.origin, rotated_shape);
        update_vert_contour(vert_contour, lchild.origin, rotated_shape);
    }
    else {
        update_horz_contour(horz_contour, lchild.origin, lchild.shape);
        update_vert_contour(vert_contour, lchild.origin, lchild.shape);
    }
}

static void place_right_child2(std::vector<unsigned int> &horz_contour,std::vector<unsigned int> &vert_contour,
module_t & parent,bool parent_rotated, module_t &rchild, bool child_rotated){
    int x = parent.origin.x;
    int y = max_contour(horz_contour, x, x+(child_rotated?rchild.shape.h:rchild.shape.w));
    rchild.origin.set(x,y);
    shape_t rotated_shape(rchild.shape.h, rchild.shape.w);
    if(child_rotated){
        update_horz_contour(horz_contour, rchild.origin, rotated_shape);
        update_vert_contour(vert_contour, rchild.origin, rotated_shape);
    }
    else {
        update_horz_contour(horz_contour, rchild.origin, rchild.shape);
        update_vert_contour(vert_contour, rchild.origin, rchild.shape);
    }
}

shape_t b_node_t::pack(boost::shared_ptr<b_node_t>root,std::vector<module_t> & module_array, std::vector<unsigned int> &horz_contour,std::vector<unsigned int>&vert_contour){
    std::queue<b_node_t> bfs_queue;
    global_var_t *global_var = global_var_t::get_ref();

    init_contour(horz_contour, global_var->get_die_shape().w*2);
    init_contour(vert_contour, global_var->get_die_shape().w*2);

    bfs_queue.push(*root);
    ///< place root node
    module_array[root->module_id].origin.set(0,0);
    ///< update contour
    update_horz_contour(horz_contour,module_array[root->module_id].origin, module_array[root->module_id].shape);
    update_vert_contour(vert_contour,module_array[root->module_id].origin, module_array[root->module_id].shape);


    while(!bfs_queue.empty()){
        b_node_t sub_root = bfs_queue.front();
        unsigned int poped_id = sub_root.module_id;
        bfs_queue.pop();
        boost::shared_ptr<b_node_t> lchild = sub_root.lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root.rchild;
        if(lchild!=NULL){
            unsigned int child_id = lchild->module_id;
            place_left_child(horz_contour, vert_contour, module_array[poped_id], module_array[child_id]);
            bfs_queue.push(*lchild);

        }
        if(rchild!=NULL){
            unsigned int child_id = rchild->module_id;
            place_right_child(horz_contour, vert_contour, module_array[poped_id], module_array[child_id]);
            bfs_queue.push(*rchild);
        }
    }
    shape_t result;
    result.h = *std::max_element(horz_contour.begin(), horz_contour.end());
    result.w = *std::max_element(vert_contour.begin(), vert_contour.end());

    return result;
}

shape_t b_node_t::pack2(boost::shared_ptr<b_node_t>root,std::vector<module_t> & module_array){
    std::queue<b_node_t> bfs_queue;
    std::vector<unsigned int> horz_contour;
    std::vector<unsigned int> vert_contour;
    global_var_t *global_var = global_var_t::get_ref();

    init_contour(horz_contour, global_var->get_die_shape().w*2);
    init_contour(vert_contour, global_var->get_die_shape().w*2);

    bfs_queue.push(*root);
    ///< place root node
    module_array[root->module_id].origin.set(0,0);
    ///< update contour
    if(root->rotated){
        shape_t rotated_shape(module_array[root->module_id].shape.h, module_array[root->module_id].shape.w); 
        update_horz_contour(horz_contour,module_array[root->module_id].origin, rotated_shape);
        update_vert_contour(vert_contour,module_array[root->module_id].origin, rotated_shape);
    }
    else {
        update_horz_contour(horz_contour,module_array[root->module_id].origin, module_array[root->module_id].shape);
        update_vert_contour(vert_contour,module_array[root->module_id].origin, module_array[root->module_id].shape);
    }


    while(!bfs_queue.empty()){
        b_node_t sub_root = bfs_queue.front();
        unsigned int poped_id = sub_root.module_id;
        bfs_queue.pop();
        boost::shared_ptr<b_node_t> lchild = sub_root.lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root.rchild;
        if(lchild!=NULL){
            unsigned int child_id = lchild->module_id;
            place_left_child2(horz_contour, vert_contour, module_array[poped_id],sub_root.rotated,
             module_array[child_id], lchild->rotated);
            bfs_queue.push(*lchild);

        }
        if(rchild!=NULL){
            unsigned int child_id = rchild->module_id;
            place_right_child2(horz_contour, vert_contour, module_array[poped_id],sub_root.rotated,
             module_array[child_id], rchild->rotated);
            bfs_queue.push(*rchild);
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
    std::queue<b_node_t> bfs_queue;
    
    bfs_queue.push(*root);
    int overlap_count=0;
    
    while(!bfs_queue.empty()){
        b_node_t sub_root = bfs_queue.front();
        unsigned int poped_id = sub_root.module_id;
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

        boost::shared_ptr<b_node_t> lchild = sub_root.lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root.rchild;
        if(lchild!=NULL){
            unsigned int child_id = lchild->module_id;
            bfs_queue.push(*lchild);

        }
        if(rchild!=NULL){
            unsigned int child_id = rchild->module_id;
            bfs_queue.push(*rchild);
        }
    }    
    return overlap_count==0?false:true;

}

void b_node_t::swap(boost::shared_ptr<b_node_t> a, boost::shared_ptr<b_node_t> b){
    unsigned int tmp = a->module_id;
    bool rotated=a->rotated;
    a->module_id=b->module_id; b->module_id=tmp;
    a->rotated=b->rotated; b->rotated=rotated;
   
/*
    boost::shared_ptr<b_node_t> parent_of_a = a->parent;
    boost::shared_ptr<b_node_t> parent_of_b = b->parent;
    boost::shared_ptr<b_node_t> lchild_of_a = a->lchild;
    boost::shared_ptr<b_node_t> rchild_of_a = a->rchild;
    boost::shared_ptr<b_node_t> lchild_of_b = b->lchild;
    boost::shared_ptr<b_node_t> rchild_of_b = b->rchild;

    boost::shared_ptr<b_node_t> tmp = b;
    bool a_is_lchild = a->is_as_lchild();
    bool b_is_lchild = b->is_as_lchild();
    if(a_is_lchild) parent_of_a->lchild = b; else parent_of_a->rchild=b;
    if(b_is_lchild) parent_of_b->lchild = a; else parent_of_b->rchild=a;

    a->parent = parent_of_b;
    b->parent = parent_of_a;
    a->lchild = lchild_of_b;
    a->rchild = rchild_of_b;
    if(a->lchild!=NULL) a->lchild->parent = a;
    if(a->rchild!=NULL) a->rchild->parent = a;
    b->lchild = lchild_of_a;
    b->rchild = rchild_of_a;
    if(b->lchild!=NULL) b->lchild->parent = b;
    if(b->rchild!=NULL) b->rchild->parent = b;
    std::cout<<"end of swap"<<a->module_id<<", "<<b->module_id<<std::endl;
*/
}
void b_node_t::move(boost::shared_ptr<b_node_t>  a, boost::shared_ptr<b_node_t> dst_root){
    int value = a->num_of_children();
    if(dst_root->num_of_children()>1)
        return;
    if(a->parent==dst_root)
        return;
    BOOST_ASSERT_MSG(a->is_as_root()==false, "illegal instruction");
    if(value==1){
        boost::shared_ptr<b_node_t> child;
        if(a->lchild != NULL) child=a->lchild;
        else child = a->rchild;
        if(a->is_as_lchild()){
            a->parent->lchild=child;
            if(child!=NULL) child->parent=a->parent;
        }
        else if(a->is_as_rchild()){
            a->parent->rchild=child;
            if(child!=NULL) child->parent=a->parent;
        }
        a->lchild.reset();
        a->rchild.reset();
        if(dst_root->lchild==NULL) { dst_root->lchild=a; a->parent=dst_root;}
        else if(dst_root->rchild==NULL) { dst_root->rchild=a; a->parent=dst_root;}
    }
    else if(value==2){
        ///< TODO to implement this
    }

}


boost::shared_ptr<b_node_t> b_node_t::tree_copy(boost::shared_ptr<b_node_t> src_root){
    
    boost::shared_ptr<b_node_t> dst_root(new b_node_t(0));
    std::queue<b_node_t> bfs_queue;
    std::queue<boost::shared_ptr<b_node_t> > construct_queue;
    
    bfs_queue.push(*src_root);    
    *dst_root = *src_root;
    construct_queue.push(dst_root);

    while(!bfs_queue.empty()){
        b_node_t sub_root = bfs_queue.front();
        bfs_queue.pop();
        boost::shared_ptr<b_node_t> ptr_current = construct_queue.front();
        construct_queue.pop();
        
        boost::shared_ptr<b_node_t> lchild = sub_root.lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root.rchild;
        if(lchild!=NULL){
            bfs_queue.push(*lchild);
            ptr_current->lchild = boost::shared_ptr<b_node_t> (new b_node_t(0));
            *ptr_current->lchild = *lchild;
            construct_queue.push(ptr_current->lchild);
        }
        else ptr_current->lchild.reset();
        if(rchild!=NULL){
            bfs_queue.push(*rchild);
            ptr_current->rchild = boost::shared_ptr<b_node_t> (new b_node_t(0));
            *ptr_current->rchild = *rchild;
            construct_queue.push(ptr_current->rchild);
        }
        else ptr_current->rchild.reset();
    }    
    return dst_root;

}


