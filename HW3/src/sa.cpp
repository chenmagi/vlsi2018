#include "sa.hpp"
#include <queue>
#include <boost/foreach.hpp>
#include "algo.hpp"
#include <iostream>
#include "globalvar.hpp"


solution_t simulated_annealing_t::get_next_solution(std::vector<module_t> & module_array, solution_t &cur_sol){
    
    solution_t result;
    result = cur_sol;
    
    random_t::dice_t op=random_t::get_ref().rolling();
    int num_of_modules = module_array.size();
    int random_id0 = floor(rand()% (num_of_modules));
    int random_id1;
    int tries = 4;
    do{
        random_id1 = floor(rand()% (num_of_modules));
    }while(random_id0!=random_id1 && tries-->0);
    if(random_id0==random_id1){
        random_id1=(random_id0+3)%num_of_modules;
    }

    boost::shared_ptr<b_node_t> node1 = result.lookup_tbl[random_id0];   
    boost::shared_ptr<b_node_t> node2 = result.lookup_tbl[random_id1];   

    
    
    switch(op){
        case random_t::dice_t::FIRST_DEVIATION: ///< rotate
            b_node_t::swap(node1, node2);
        break;
        case random_t::dice_t::SECOND_DEVIATION: ///< swap
            node1->rotate();
        break;
        
        case random_t::dice_t::THIRD_DEVIATION: ///< move
            node2->rotate();
            b_node_t::swap(node2, node1);
        break;
        
        
    }    
    int ret_count=0;
    
    /*
    int num_of_nodes = 0;
    std::ostringstream ostream;
    build_graphviz(result.tree_root, ostream, &num_of_nodes);
    std::cout<<" num of nodes in copy version of b-tree: "<< num_of_nodes << std::endl;
    std::cout << ostream.str() <<std::endl;
    */
    b_node_t::dfs_visit(result.tree_root, &ret_count);
    BOOST_ASSERT(module_array.size()==ret_count);
    

    return result;
}

void simulated_annealing_t::run(std::vector<module_t> & module_array, std::vector<net_t> &net_array, std::vector<terminal_t> &pin_array,int timeout){
    double tc_current = this->tc_start;
    int node_count=0;
    std::ostringstream ostream;
    std::vector<tiny_module_t> sorted_array;
    int rc = build_sorted_module_array(module_array, sorted_array);

    boost::shared_ptr<b_node_t> init_tree(new b_node_t(0));
    build_b_tree(module_array, sorted_array, init_tree);

    //std::vector<unsigned int> h_contour, v_contour;
    shape_t init_shape=b_node_t::pack2(init_tree, module_array);//, h_contour, v_contour);
    std::cout << "init shape="<<init_shape.w <<" x "<<init_shape.h <<std::endl;
    
    
    cur_sol.build_from_b_tree(init_tree, module_array.size());
    cur_sol.update_cost(module_array,net_array, pin_array);
    best_sol = cur_sol;
    std::cout << "initial solution: "<<cur_sol.toString()<<std::endl;

    global_var_t *global_var = global_var_t::get_ref();
    shape_t target_die_shape = global_var->get_die_shape();
    int iteration=0;
    int tune=0;
    while(tc_current > this->tc_end && timer_t::get_ref().elapsed() < timeout){
        double old_cost = cur_sol.cost;
        solution_t new_solution = get_next_solution(module_array, cur_sol);
        new_solution.update_cost(module_array, net_array, pin_array);
        if(new_solution.die_shape.w <= target_die_shape.w && new_solution.die_shape.h <= target_die_shape.h){
            best_sol = new_solution;
            std::cout<<"find fit solution = "<<new_solution.die_shape.w <<" x " <<new_solution.die_shape.w<<std::endl;
            break;
        }

        double prob = acceptance(new_solution.cost, old_cost, tc_current);
        if(prob>=0.25){
            cur_sol = new_solution;
            if(new_solution.cost < best_sol.cost)
                best_sol = new_solution;
            tune=0;    
        }
        else {
            tune++;
        }
        tc_current *= this->cooling_factor;
        if(tune>=10000) cur_sol = best_sol;
        std::cout<<"["<<iteration++<<"]"<<cur_sol.toString() << std::endl;
    }
    if(timer_t::get_ref().elapsed() > timeout){
        std::cout<<"time out"<<std::endl;
    }
    return;


}



void solution_t::build_from_b_tree(boost::shared_ptr<b_node_t> src_root, int num_of_nodes){
    this->tree_root = boost::shared_ptr<b_node_t>(new b_node_t(src_root->module_id));
    std::queue<b_node_t> bfs_queue;
    std::queue<boost::shared_ptr<b_node_t> > construct_queue;

    lookup_tbl.clear();
    for(int i=0;i<num_of_nodes;++i){
        lookup_tbl.push_back(0);
    }
    
    bfs_queue.push(*src_root);    
    
    construct_queue.push(tree_root);

    while(!bfs_queue.empty()){
        b_node_t sub_root = bfs_queue.front();
        bfs_queue.pop();
        boost::shared_ptr<b_node_t> ptr_current = construct_queue.front();
        construct_queue.pop();
        lookup_tbl[sub_root.module_id]=ptr_current;
        
        boost::shared_ptr<b_node_t> lchild = sub_root.lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root.rchild;
        if(lchild!=NULL){
            bfs_queue.push(*lchild);
            ptr_current->lchild = boost::shared_ptr<b_node_t> (new b_node_t(0));
            *ptr_current->lchild = *lchild;
            ptr_current->lchild->parent = ptr_current;
            construct_queue.push(ptr_current->lchild);
        }
        else ptr_current->lchild = NULL;
        if(rchild!=NULL){
            bfs_queue.push(*rchild);
            ptr_current->rchild = boost::shared_ptr<b_node_t> (new b_node_t(0));
            *ptr_current->rchild = *rchild;
            ptr_current->rchild->parent = ptr_current;
            construct_queue.push(ptr_current->rchild);
        }
        else ptr_current->rchild = NULL;
    }    

    return;
}

static bool integer_cmp( int a,  int b){
    return a < b;
}

void solution_t::update_cost(std::vector<module_t> &module_array, std::vector<net_t> &net_array, std::vector<terminal_t> &pin_array){
    std::vector<int> x_array;
    std::vector<int> y_array;
    cost =0;
    double length=0;
    //std::vector<unsigned int> h_contour, v_contour;
    die_shape=b_node_t::pack2(tree_root, module_array);//, h_contour, v_contour);
    BOOST_FOREACH(auto net, net_array){
        for(int i=0;i<net.module_ids.size();++i){
            unsigned int id = net.module_ids[i];
            unsigned int cx, cy;
            bool is_rotated=this->lookup_tbl[id]->rotated;
            if(is_rotated)
                module_array[id].get_rotated_centric(&cx, &cy);
            else module_array[id].get_centric(&cx, &cy);
            x_array.push_back((int)cx);
            y_array.push_back((int)cy);
        }
        if(net.pin_count==1){
            unsigned int px, py;
            px = pin_array[net.pin_id].coord.x;
            py = pin_array[net.pin_id].coord.y;
            x_array.push_back((int)px);
            y_array.push_back((int)py);
        }
        sort(x_array.begin(), x_array.end(), integer_cmp);
        sort(y_array.begin(), y_array.end(), integer_cmp);
        
        int dx=x_array[x_array.size()-1]-x_array[0];
        int dy=y_array[y_array.size()-1]-y_array[0];
        
        length += dx;
        length += dy;
        x_array.clear(); y_array.clear();
    }    
    length *= this->alpha;
    cost = die_shape.area() + length;
    return;

}